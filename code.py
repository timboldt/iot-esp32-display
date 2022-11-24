import time
import board
import displayio
import terminalio
from adafruit_display_text import label
from adafruit_display_shapes.sparkline import Sparkline
import adafruit_il0373

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

g = displayio.Group()

bg = displayio.Bitmap(display.width, display.height, 1)
bg_palette = displayio.Palette(1)
bg_palette[0] = 0xFFFFFF
g.append(displayio.TileGrid(bg, pixel_shader=bg_palette, x=0, y=0))

text_area = label.Label(font, text="Hello World!", color=black)
text_area.x = 100
text_area.y = 80
g.append(text_area)

graph = Sparkline(width=display.width, height=display.height, max_items=40, y_min=0, y_max=10, x=0, y=0, color=red)
graph.add_value(3)
graph.add_value(1)
graph.add_value(4)
graph.add_value(1)
graph.add_value(5)
graph.add_value(9)
g.append(graph)

display.show(g)

display.refresh()
print("refreshed")

time.sleep(180)
