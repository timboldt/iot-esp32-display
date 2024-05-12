mod secrets;

use anyhow::{bail, Result};
use core::str;
use embedded_graphics::{pixelcolor::BinaryColor, Pixel};
use embedded_hal::spi::MODE_0;
use embedded_svc::{
    http::{client::Client, Method},
    io::Read,
};
use epd_waveshare::{
    epd4in2::{Display4in2, Epd4in2},
    prelude::*,
};
use esp_idf_hal::{
    gpio::{AnyOutputPin, PinDriver},
    peripheral,
    prelude::Peripherals,
    spi::{config::Config, SpiDeviceDriver, SpiDriver, SpiDriverConfig, SPI2},
    units::FromValueType,
};
use esp_idf_svc::{
    eventloop::EspSystemEventLoop,
    http::client::{Configuration, EspHttpConnection},
    wifi::{AuthMethod, BlockingWifi, ClientConfiguration, EspWifi},
};
use esp_idf_svc::{hal::delay::Ets, wifi::Configuration as WifiConfiguration};
use log::info;
use tinybmp::Bmp;

fn main() -> Result<()> {
    esp_idf_svc::sys::link_patches();
    esp_idf_svc::log::EspLogger::initialize_default();

    let peripherals = Peripherals::take().unwrap();
    let sysloop = EspSystemEventLoop::take()?;

    // Connect to the Wi-Fi network.
    let _wifi = wifi(
        secrets::WIFI_SSID,
        secrets::WIFI_PASSWORD,
        peripherals.modem,
        sysloop,
    )?;

    let sclk = peripherals.pins.gpio5;
    let sdo = peripherals.pins.gpio19;
    let sdi = peripherals.pins.gpio21;
    let cs = PinDriver::output(peripherals.pins.gpio15)?;
    let busy = PinDriver::input(peripherals.pins.gpio14)?;
    let dc = PinDriver::output(peripherals.pins.gpio33)?;
    let rst = PinDriver::output(peripherals.pins.gpio32)?;
    let mut delay = Ets;

    let driver = SpiDriver::new::<SPI2>(
        peripherals.spi2,
        sclk,
        sdo,
        Some(sdi),
        &SpiDriverConfig::new(),
    )?;
    let config = Config::new().baudrate(4.MHz().into()).data_mode(MODE_0);
    let mut spi_device = SpiDeviceDriver::new(&driver, AnyOutputPin::none(), &config)?;

    // Setup the epd
    let mut epd4in2 =
        Epd4in2::new(&mut spi_device, cs, busy, dc, rst, &mut delay).expect("eink initalize error");

    // Setup the graphics buffer.
    let mut display = Display4in2::default();
    // Load the image into the buffer.
    get(
        format!("{}/epd42bw.bmp", secrets::BUCKET_URL),
        display.get_mut_buffer(),
    )?;
    // Output it to the actual epaper.
    epd4in2.update_and_display_frame(&mut spi_device, display.buffer(), &mut delay)?;
    // Turn off the epaper display's power.
    epd4in2.sleep(&mut spi_device, &mut delay)?;

    Ok(())
}

pub fn wifi(
    ssid: &str,
    pass: &str,
    modem: impl peripheral::Peripheral<P = esp_idf_svc::hal::modem::Modem> + 'static,
    sysloop: EspSystemEventLoop,
) -> Result<Box<EspWifi<'static>>> {
    let mut auth_method = AuthMethod::WPA2Personal;
    if ssid.is_empty() {
        bail!("Missing WiFi name")
    }
    if pass.is_empty() {
        auth_method = AuthMethod::None;
        info!("Wifi password is empty");
    }
    let mut esp_wifi = EspWifi::new(modem, sysloop.clone(), None)?;

    let mut wifi = BlockingWifi::wrap(&mut esp_wifi, sysloop)?;

    wifi.set_configuration(&WifiConfiguration::Client(ClientConfiguration::default()))?;

    info!("Starting wifi...");

    wifi.start()?;

    info!("Scanning...");

    let ap_infos = wifi.scan()?;

    let ours = ap_infos.into_iter().find(|a| a.ssid == ssid);

    let channel = if let Some(ours) = ours {
        info!(
            "Found configured access point {} on channel {}",
            ssid, ours.channel
        );
        Some(ours.channel)
    } else {
        info!(
            "Configured access point {} not found during scanning, will go with unknown channel",
            ssid
        );
        None
    };

    wifi.set_configuration(&WifiConfiguration::Client(ClientConfiguration {
        ssid: ssid.try_into().unwrap_or_default(),
        password: pass.try_into().unwrap_or_default(),
        channel,
        auth_method,
        ..Default::default()
    }))?;

    info!("Connecting wifi...");

    wifi.connect()?;

    info!("Waiting for DHCP lease...");

    wifi.wait_netif_up()?;

    let ip_info = wifi.wifi().sta_netif().get_ip_info()?;

    info!("Wifi DHCP info: {:?}", ip_info);

    Ok(Box::new(esp_wifi))
}

fn get(url: impl AsRef<str>, display_buffer: &mut [u8]) -> Result<()> {
    static mut BMP_BUF: [u8; 100000] = [0; 100000];

    #[allow(static_mut_refs)]
    let buffer = unsafe { &mut BMP_BUF };

    // 1. Create a new EspHttpClient. (Check documentation)
    // ANCHOR: connection
    let connection = EspHttpConnection::new(&Configuration {
        use_global_ca_store: true,
        crt_bundle_attach: Some(esp_idf_svc::sys::esp_crt_bundle_attach),
        ..Default::default()
    })?;
    // ANCHOR_END: connection
    let mut client = Client::wrap(connection);

    // 2. Open a GET request to `url`
    let headers = [("accept", "application/octet-stream")];
    let request = client.request(Method::Get, url.as_ref(), &headers)?;

    // 3. Submit write request and check the status code of the response.
    // Successful http status codes are in the 200..=299 range.
    let response = request.submit()?;
    let status = response.status();

    info!("Response code: {}\n", status);

    match status {
        200..=299 => {
            let mut offset = 0;
            let mut total = 0;
            let mut reader = response;
            loop {
                if let Ok(size) = Read::read(&mut reader,&mut buffer[offset..]) {
                    if size == 0 {
                        break;
                    }
                    offset += size;
                    total += size;
                }
            }
            info!("Total: {} bytes", total);
        }
        _ => bail!("Unexpected response code: {}", status),
    }

    if let Ok(bmp) = Bmp::<BinaryColor>::from_slice(buffer) {
        for Pixel(position, color) in bmp.pixels() {
            if position.x < 400 && position.y < 300 {
                let byte_loc = ((position.x + position.y * 400) / 8) as usize;
                let bit = 1u8 << (7 - position.x % 8);
                match color {
                    BinaryColor::On => display_buffer[byte_loc] |= bit,
                    BinaryColor::Off => display_buffer[byte_loc] &= !bit,
                };
            }
        }
    } else {
        bail!("Unexpected BMP format");
    }

    Ok(())
}
