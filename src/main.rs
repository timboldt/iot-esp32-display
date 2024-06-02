#![no_std]
#![no_main]

use epd_waveshare::{
    epd4in2::{Display4in2, Epd4in2},
    prelude::*,
};
use esp_backtrace as _;
use esp_hal::{
    clock::ClockControl,
    delay::Delay,
    gpio::{IO, NO_PIN},
    peripherals::Peripherals,
    prelude::*,
    spi::{
        master::{self, Spi},
        SpiMode,
    },
};
//use il0373::{Builder, Color, Dimensions, Display, GraphicDisplay, Interface, Rotation};

#[entry]
fn main() -> ! {
    let peripherals = Peripherals::take();
    let system = peripherals.SYSTEM.split();
    let clocks = ClockControl::boot_defaults(system.clock_control).freeze();
    let io = IO::new(peripherals.GPIO, peripherals.IO_MUX);

    // Set up SPI bus for EPD.
    let sclk = io.pins.gpio5;
    let mosi = io.pins.gpio19;
    let miso = io.pins.gpio21;
    let cs = io.pins.gpio15.into_push_pull_output();
    let busy = io.pins.gpio14.into_floating_input();
    let dc = io.pins.gpio33.into_push_pull_output();
    let rst = io.pins.gpio32.into_push_pull_output();

    let mut spi = Spi::new(peripherals.SPI2, 4.MHz(), SpiMode::Mode0, &clocks).with_pins(
        Some(sclk),
        Some(mosi),
        Some(miso),
        NO_PIN,
    );

    let mut delay = Delay::new(&clocks);

    esp_println::logger::init_logger_from_env();

    let mut epd4in2 =
        Epd4in2::new(&mut spi, cs, busy, dc, rst, &mut delay).expect("could not init EPD4in2");

    // Setup the graphics buffer.
    let mut display = Display4in2::default();

    // let _init = esp_wifi::initialize(
    //     esp_wifi::EspWifiInitFor::Wifi,
    //     timer,
    //     esp_hal::rng::Rng::new(peripherals.RNG),
    //     system.radio_clock_control,
    //     &clocks,
    // )
    // .unwrap();

    // // Connect to the Wi-Fi network.
    // let _wifi = wifi(
    //     secrets::WIFI_SSID,
    //     secrets::WIFI_PASSWORD,
    //     peripherals.modem,
    //     sysloop,
    // )?;

    // Load the image into the buffer.
    // get(
    //     format!("{}/epd42bw.bmp", secrets::BUCKET_URL),
    //     display.get_mut_buffer(),
    // )?;

    // Output it to the actual epaper.
    epd4in2.update_and_display_frame(&mut spi, display.buffer(), &mut delay)?;
    // Turn off the epaper display's power.
    epd4in2.sleep(&mut spi, &mut delay)?;

    loop {
        log::info!("Hello world!");
        delay.delay(500.millis());
    }
}
