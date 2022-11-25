import analogio
import board
import displayio
import os
import socketpool
import supervisor
import terminalio
import time
import wifi
from adafruit_display_text import label
from adafruit_display_shapes.sparkline import Sparkline
import adafruit_il0373
import adafruit_requests


def get_time(http):
    resp = http.get("http://io.adafruit.com/api/v2/{}/integrations/time/strftime?fmt=%25I:%25M:%25S%20%25p".format(
        aio_username), headers={"X-AIO-Key": aio_key})
    t = resp.text
    resp.close()
    return t


def fetch_feed(http, aio_username, aio_key, feed):
    resp = http.get("http://io.adafruit.com/api/v2/{}/feeds/{}/data/chart?hours=48&resolution=10".format(
        aio_username, feed), headers={"X-AIO-Key": aio_key})
    json = resp.json()
    resp.close()
    return json


def make_sparkline(data, color):
    text_gap = 15
    graph = Sparkline(width=display.width, height=display.height-text_gap,
                      max_items=len(data), x=0, y=0, color=color)
    for d in data:
        # Data value is the second value in the structure.
        graph.add_value(float(d[1]), False)
    graph.update()
    return graph


font = terminalio.FONT
black = 0x0000FF
white = 0xFFFFFF
red = 0xFF0000

displayio.release_displays()

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

batt_pin = analogio.AnalogIn(board.D35)
voltage = round(batt_pin.value * 3.3 * 2 / 65536, 1)

aio_username = os.getenv("AIO_USERNAME")
aio_key = os.getenv("AIO_KEY")
decimals = 1

print("Getting data...")

pool = socketpool.SocketPool(wifi.radio)
http = adafruit_requests.Session(pool)

g = displayio.Group()

bg = displayio.Bitmap(display.width, display.height, 1)
bg_palette = displayio.Palette(1)
bg_palette[0] = 0xFFFFFF
g.append(displayio.TileGrid(bg, pixel_shader=bg_palette, x=0, y=0))

status = label.Label(font, text="{} {}".format(voltage, get_time(http)), color=black)
status.x = 0
status.y = 5
g.append(status)

json = fetch_feed(http, aio_username, aio_key, "finance.coinbase-btc-usd")
if len(json["data"]) > 0:
    graph = make_sparkline(json["data"], black)
    g.append(graph)
    title = label.Label(font, text="{} {}".format(json["feed"]["name"], json["data"][-1][1]), color=black)
    title.x = 0
    title.y = display.height - 5
    g.append(title)

json = fetch_feed(http, aio_username, aio_key, "finance.kraken-usdtzusd")
if len(json["data"]) > 0:
    graph = make_sparkline(json["data"], red)
    g.append(graph)
    title = label.Label(font, text="{} {}".format(json["feed"]["name"], json["data"][-1][1]), color=red)
    title.x = int(display.width / 2)
    title.y = display.height - 5
    g.append(title)

display.show(g)

display.refresh()
print("refreshed")

time.sleep(180)
supervisor.reload()
