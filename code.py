import alarm
import analogio
import board
import digitalio
import displayio
import os
import socketpool
import ssl
import supervisor
import terminalio
import time
import wifi
from adafruit_display_text import label
from adafruit_display_shapes.sparkline import Sparkline
import adafruit_il0373
from adafruit_io.adafruit_io import IO_HTTP
import adafruit_requests


def get_time(http):
    resp = http.get("http://io.adafruit.com/api/v2/{}/integrations/time/strftime?fmt=%25I:%25M:%25S%20%25p".format(
        aio_username), headers={"X-AIO-Key": aio_key})
    t = resp.text
    resp.close()
    return t


def fetch_feed(http, aio_username, aio_key, feed):
    resp = http.get("https://io.adafruit.com/api/v2/{}/feeds/{}/data/chart?hours=168&resolution=60".format(
        aio_username, feed), headers={"X-AIO-Key": aio_key})
    json = resp.json()
    resp.close()
    return json


def make_graph(json, color, offset):
    data = json["data"]
    
    g = displayio.Group()

    text_gap = 15
    graph = Sparkline(width=int(display.width / 2), height=display.height-text_gap,
                      max_items=len(data), x=offset*int(display.width / 2), y=0, color=color)
    for d in data:
        # Data value is the second value in the structure.
        graph.add_value(float(d[1]), False)
    graph.update()
    g.append(graph)

    v = float(data[-1][1])
    fmt = "{} {}"
    if v > 2000:
        v /= 1000
        fmt = "{} {}k"
    if v < 10:
        v = round(v, 4)
    else:
        v = round(v, 2)
    title = label.Label(font, text=fmt.format(
        json["feed"]["name"], v), color=color)
    title.x = offset*int(display.width / 2)
    title.y = display.height - 5
    g.append(title)

    return g

# Constants.
font = terminalio.FONT
black = 0x0000FF
white = 0xFFFFFF
red = 0xFF0000

displayio.release_displays()

# Turn of I2C/Neopixel to save power.
power_pin = digitalio.DigitalInOut(board.NEOPIXEL_I2C_POWER)
power_pin.direction = digitalio.Direction.OUTPUT
power_pin.value = False

spi = board.SPI()
epd_cs = board.D15
epd_dc = board.D33
epd_reset = None
epd_busy = None

display_bus = displayio.FourWire(
    spi, command=epd_dc, chip_select=epd_cs, reset=epd_reset, baudrate=1000000
)
time.sleep(1)

display = adafruit_il0373.IL0373(
    display_bus,
    width=296,
    height=128,
    rotation=270,
    busy_pin=epd_busy,
    highlight_color=red,
)

aio_username = os.getenv("AIO_USERNAME")
aio_key = os.getenv("AIO_KEY")
decimals = 1

print("Getting data...")

pool = socketpool.SocketPool(wifi.radio)
http = adafruit_requests.Session(pool, ssl_context=ssl.create_default_context())

batt_pin = analogio.AnalogIn(board.VOLTAGE_MONITOR)
voltage = batt_pin.value * 3.3 * 2 / 65536
io = IO_HTTP(aio_username, aio_key, http)
voltage_feed = io.get_feed("tricolor-battery")
io.send_data(voltage_feed["key"], voltage)

g = displayio.Group()

bg = displayio.Bitmap(display.width, display.height, 1)
bg_palette = displayio.Palette(1)
bg_palette[0] = 0xFFFFFF
g.append(displayio.TileGrid(bg, pixel_shader=bg_palette, x=0, y=0))

status = label.Label(font, text="{} {}".format(
    round(voltage, 1), get_time(http)), color=black)
status.x = 0
status.y = 5
g.append(status)

json = fetch_feed(http, aio_username, aio_key, "finance.coinbase-btc-usd")
graph = make_graph(json, red, 0)
g.append(graph)

json = fetch_feed(http, aio_username, aio_key, "finance.kraken-usdtzusd")
graph = make_graph(json, black, 1)
g.append(graph)

display.show(g)

display.refresh()
print("refreshed")

if voltage > 4.15:
    # Light sleep for 3 minutes.
    time.sleep(180)
else:
    print("Entering deep sleep in a few seconds...")
    time.sleep(10)
    # Deep sleep for a few minutes.
    time_alarm = alarm.time.TimeAlarm(monotonic_time=time.monotonic() + 15*60)
    alarm.exit_and_deep_sleep_until_alarms(time_alarm)
    # In theory, the code won't get to this point, because deep sleep causes a reset.

supervisor.reload()
