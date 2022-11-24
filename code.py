import time
import board
import displayio
import os
import socketpool
import supervisor
import terminalio
import wifi
from adafruit_display_text import label
from adafruit_display_shapes.sparkline import Sparkline
import adafruit_il0373
import adafruit_requests

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

aio_username = os.getenv("AIO_USERNAME")
aio_key = os.getenv("AIO_KEY")
aio_feed = "finance.coinbase-btc-usd"
decimals = 1

print("Getting data...")

pool = socketpool.SocketPool(wifi.radio)
http = adafruit_requests.Session(pool)
resp = http.get("http://io.adafruit.com/api/v2/{}/feeds/{}/data/chart?hours=48&resolution=10".format(
    aio_username, aio_feed), headers={"X-AIO-Key": aio_key})
json = resp.json()
resp.close()

title = json["feed"]["name"]
latest = "NO DATA"
if len(json["data"]) > 0 and len(json["data"][-1]) >= 2:
    latest = round(float(json["data"][-1][1]), decimals)

print("{}: {}".format(title, latest))
print("{} data points".format(len(json["data"])))

g = displayio.Group()

bg = displayio.Bitmap(display.width, display.height, 1)
bg_palette = displayio.Palette(1)
bg_palette[0] = 0xFFFFFF
g.append(displayio.TileGrid(bg, pixel_shader=bg_palette, x=0, y=0))

text_area = label.Label(font, text="{}: {}".format(title, latest), color=black)
text_area.x = 0
text_area.y = 5
g.append(text_area)

top_gap = 15
graph = Sparkline(width=display.width, height=display.height-top_gap,
                  max_items=len(json["data"]), x=0, y=top_gap, color=black)
for d in json["data"]:
    graph.add_value(float(d[1]), False)
graph.update()
g.append(graph)

display.show(g)

display.refresh()
print("refreshed")

time.sleep(180)
supervisor.reload()
