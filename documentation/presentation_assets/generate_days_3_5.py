from pathlib import Path
from PIL import Image, ImageDraw, ImageFont


OUT = Path(__file__).resolve().parent
W, H = 1600, 900
TEAL = "#07939A"
DARK = "#17343A"
MUTED = "#66777B"
LINE = "#A9D7DA"
BG = "#FFFFFF"


def font(size: int, bold: bool = False):
    name = "arialbd.ttf" if bold else "arial.ttf"
    return ImageFont.truetype(str(Path("C:/Windows/Fonts") / name), size)


F_TITLE_DAY = font(42, True)
F_TITLE = font(40, True)
F_HEAD = font(25, True)
F_BODY = font(24)
F_FOOT = font(17)


def wrap(draw, text, fnt, max_width):
    words = text.split()
    lines, current = [], ""
    for word in words:
        trial = word if not current else current + " " + word
        if draw.textlength(trial, font=fnt) <= max_width:
            current = trial
        else:
            if current:
                lines.append(current)
            current = word
    if current:
        lines.append(current)
    return lines


def bullets(draw, items, x, y, width, line_gap=9, item_gap=15):
    for item in items:
        draw.ellipse((x, y + 11, x + 7, y + 18), fill=TEAL)
        lines = wrap(draw, item, F_BODY, width - 28)
        for line in lines:
            draw.text((x + 24, y), line, font=F_BODY, fill=DARK)
            y += 33
        y += item_gap
    return y


def section(draw, title, x, y, width):
    draw.text((x, y), title, font=F_HEAD, fill=DARK)
    line_y = y + 39
    draw.line((x, line_y, x + width, line_y), fill=TEAL, width=3)
    return line_y + 17


SLIDES = [
    {
        "day": "Day 3",
        "title": "I2C Study and Hardware Preparation",
        "date": "03 Sep 2026",
        "page": "5",
        "tasks": [
            "Deepened my understanding of the I2C protocol and shared-bus communication.",
            "Redesigned the circuit to include push buttons and LEDs.",
            "Prepared initial sensor test code so the modules could be tested immediately after delivery.",
            "Started exploring the STM32 AI workflow while waiting for the components.",
        ],
        "achievement": [
            "Completed an updated circuit design with the required buttons and LEDs.",
            "Prepared the initial software structure and sensor test code.",
            "Established a starting point for studying STM32 AI.",
        ],
        "problem": [
            "The components had not arrived, so the day was used to strengthen the design and prepare the software in advance."
        ],
    },
    {
        "day": "Day 4",
        "title": "STM32 AI Research",
        "date": "04 Sep 2026",
        "page": "6",
        "tasks": [
            "Studied the STM32 AI workflow and how an AI model is prepared for an embedded target.",
            "Learned the roles of Python, TensorFlow and model training on a computer.",
            "Mapped how a trained model can be converted and deployed using STM32Cube.AI.",
        ],
        "achievement": [
            "Built a clear PC-to-STM32 workflow for training, conversion and embedded inference.",
            "Improved my understanding of TensorFlow models and STM32Cube.AI deployment.",
        ],
        "problem": [
            "The AI workflow was new and required a full research day before practical implementation."
        ],
    },
    {
        "day": "Day 5",
        "title": "AI Example Project and Soldering",
        "date": "07 Sep 2026",
        "page": "7",
        "tasks": [
            "Tested the MNIST handwritten-digit classification project with my STM32 board.",
            "Used the example to understand model conversion and deployment through STM32Cube.AI.",
            "Received the components and began soldering the module headers.",
        ],
        "achievement": [
            "Completed a practical trial of the PC-to-MCU AI deployment workflow.",
            "Started preparing the delivered modules for individual hardware testing.",
        ],
        "problem": [
            "The example required adaptation, and soldering took longer because the required tools were limited."
        ],
    },
    {
        "day": "Day 6",
        "title": "Individual Sensor Testing",
        "date": "08 Sep 2026",
        "page": "8",
        "tasks": [
            "Tested the BME280 and resolved a supply problem by using a stable 5 V input supported by the module.",
            "Installed the OLED library and confirmed reliable display operation.",
            "Tested both PIR modules and studied their digital output, sensitivity and delay controls.",
            "Integrated the VL53L0X distance sensor, then verified the active buzzer and MCU pin assignments.",
        ],
        "achievement": [
            "Confirmed individual operation of the BME280, OLED, two PIR sensors, VL53L0X and active buzzer.",
            "Corrected the required libraries, drivers and pin configuration.",
            "Identified the practical sensor limitations before data collection.",
        ],
        "problem": [
            "The PIR outputs sometimes produced unstable or false triggers.",
            "The VL53L0X was reliable only to about 0.8 m, below the expected range.",
            "The original BH1750 failed all wiring, address and code checks and was diagnosed as faulty.",
        ],
    },
    {
        "day": "Day 7",
        "title": "System Integration and Data Templates",
        "date": "09 Sep 2026",
        "page": "9",
        "tasks": [
            "Ordered a replacement BH1750 after confirming that the original light sensor was faulty.",
            "Integrated all available sensors on the shared I2C bus while excluding the failed light sensor.",
            "Added the push buttons, LEDs and buzzer feedback step by step.",
            "Created a Python template for filtering the future dataset and outlined the AI-model workflow.",
        ],
        "achievement": [
            "Ran the available sensors and user controls together in one STM32 application.",
            "Prepared a reusable data-filtering template for the collected sensor data.",
            "Defined the next path from filtered data to AI training and STM32 deployment.",
        ],
        "problem": [
            "Waiting for the replacement BH1750 prevented final system validation, so work continued with the remaining hardware and software."
        ],
    },
    {
        "day": "Day 8",
        "title": "Final Integration and Data-Quality Review",
        "date": "10 Sep 2026",
        "page": "10",
        "tasks": [
            "Received, soldered and tested the replacement BH1750, which responded at 0x23 with valid lux readings.",
            "Integrated the complete sensor set and confirmed the devices on the shared I2C bus.",
            "Prepared the system and logging workflow for the first data-collection sessions.",
        ],
        "achievement": [
            "Completed the full hardware and firmware integration.",
            "Reached a ready-for-data-collection state with the filtering and AI templates prepared.",
            "Defined the next steps: collect, filter and train the model before STM32Cube.AI deployment.",
        ],
        "problem": [
            "PIR false triggers and the ToF sensor's short practical range and narrow field of view prevented reliable data collection.",
            "The next decision is to review sensor placement or alternatives with the supervisor before recording the final dataset.",
        ],
    },
]


def render(spec):
    img = Image.new("RGB", (W, H), BG)
    draw = ImageDraw.Draw(img)

    draw.line((70, 70, 1530, 70), fill=TEAL, width=8)
    draw.text((80, 105), spec["day"], font=F_TITLE_DAY, fill=TEAL)
    day_w = draw.textlength(spec["day"], font=F_TITLE_DAY)
    sep_x = 80 + day_w + 28
    draw.line((sep_x, 107, sep_x, 151), fill=TEAL, width=4)
    draw.text((sep_x + 28, 105), spec["title"], font=F_TITLE, fill=DARK)

    left_x, right_x = 80, 875
    left_w, right_w = 690, 645

    y = section(draw, "Tasks", left_x, 205, left_w)
    y = bullets(draw, spec["tasks"], left_x + 4, y, left_w)

    y = max(y + 10, 575)
    y = section(draw, "Learning Sources and Tools", left_x, y, left_w)
    bullets(draw, ["...", "...", "..."], left_x + 4, y, left_w, line_gap=4, item_gap=5)

    y2 = section(draw, "Achievement", right_x, 205, right_w)
    y2 = bullets(draw, spec["achievement"], right_x + 4, y2, right_w)

    y2 = max(y2 + 20, 520)
    y2 = section(draw, "Problem and Engineering Decision", right_x, y2, right_w)
    bullets(draw, spec["problem"], right_x + 4, y2, right_w)

    footer = f'Working {spec["day"]} | {spec["date"]} | Ahmed Mohamed Abdalla Ahmed | A24KE0429'
    draw.line((80, 842, 1520, 842), fill=LINE, width=2)
    draw.text((80, 856), footer, font=F_FOOT, fill=MUTED)
    page_w = draw.textlength(spec["page"], font=F_FOOT)
    draw.text((1520 - page_w, 856), spec["page"], font=F_FOOT, fill=MUTED)

    out = OUT / f'{spec["day"].lower().replace(" ", "_")}.png'
    img.save(out, quality=95)
    return out


if __name__ == "__main__":
    for slide in SLIDES:
        print(render(slide))
