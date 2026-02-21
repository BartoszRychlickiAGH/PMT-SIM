import time
import tkinter as tk
from tkinter import ttk

try:
    import serial  # pip install pyserial
except ImportError:
    serial = None


# ===================== UART do STM32 =====================

class STM32UART:
    def __init__(self, port="COM5", baudrate=115200, timeout=0.05, write_timeout=0.2):
        self.port = port
        self.baudrate = baudrate
        self.timeout = timeout
        self.write_timeout = write_timeout
        self.ser = None

    def open(self):
        if self.ser and self.ser.is_open:
            return

        self.ser = serial.Serial(
            port=self.port,
            baudrate=self.baudrate,
            timeout=self.timeout,
            write_timeout=self.write_timeout
        )
        #reset po otwarciu portu
        time.sleep(0.2)
        self.ser.reset_input_buffer()
        self.ser.reset_output_buffer()
    def is_open(self):
        return bool(self.ser and self.ser.is_open)

    def close(self):
        if self.ser.is_open:
            self.ser.close()

    def send_line(self, s):
        if not self.is_open():
            raise RuntimeError("UART nie jest otwarty")
        self.ser.write((s + "\n").encode("utf-8"))
        self.ser.flush()

    def send_slider(self, slider_id, value):
        self.send_line(f"S;{slider_id};{int(value)}")

    def send_button(self, slider_id, value):
        self.send_line(f"B;{slider_id};{int(value)}")

    def read_line(self):
        if not self.is_open():
            return None
        raw = self.ser.readline()
        if not raw:
            return None
        return raw.decode("utf-8", errors="replace").strip()


STEP = 0.1  # krok dla wszystkich sliderów

class SliderBlock(ttk.Frame):
    def __init__(self, parent, cfg, on_value_change=None):
        super().__init__(parent, padding=(0, 6))
        self.cfg = cfg
        self.on_value_change = on_value_change
        self.value_var = tk.DoubleVar(value=float(cfg["default"]))
        self.build_ui()
        self.value_var.trace_add("write", self.handle_value_change)

    def build_ui(self):
        row = ttk.Frame(self)
        row.pack(fill="x")

        ttk.Label(row, text=f'{self.cfg["id"]}', width=3).pack(side="left")
        ttk.Label(row, text=f'{self.cfg["title"]}:', width=8).pack(side="left")

        self.scale = tk.Scale(
            row,
            from_=float(self.cfg["min_val"]),
            to=float(self.cfg["max_val"]),
            orient="horizontal",
            resolution=STEP,
            showvalue=False,
            variable=self.value_var
        )
        self.scale.pack(side="left", fill="x", expand=True, padx=5)

        self.value_lbl = ttk.Label(row, text=f"{self.value_var.get():.1f}", width=6, anchor="center")
        self.value_lbl.pack(side="right")

    def handle_value_change(self, *args):  # <- zmien na *_
        v = float(self.value_var.get())
        self.value_lbl.config(text=f"{v:.1f}")

        self.on_value_change(self.cfg["id"], v) #patrze definicja w clasie app żeby tam była komunikacja
        #print("argumenty : ", args)
    # gettery/settery koniec końców nie potrezbne 
    # def get_value(self):
    #     return float(self.value_var.get())
    # def set_value(self, v):
    #     v = float(v)
    #     self.value_var.set(v)

class App(tk.Tk):
    def __init__(self):
        super().__init__()
        #pola
        self.title("Sterownik wyjścia PMT-SIM")
        self.geometry("520x300")
        self.minsize(520, 300)
        self.uart = None
        self.uart_port = "COM5"
        self.uart_baud = 115200
        self.blocks = {}
        self.status_var = tk.StringVar(value="Gotowe.")
        self.freq_on = tk.BooleanVar(value=False)
        self.freq_val = tk.StringVar(value = 0)
        self.delay_var = tk.StringVar(value = 0)
        self.toggable_wid = []
        # debounce
        self.debounce_ms = 200
        self.debounce_id = {}
        self.last_value = {}
        #Metody
        self.build_ui()
        self.try_connect_uart()
        self.on_freq_toggle()
        self.after(50, self.uart_receive)

    def build_ui(self):
        root = ttk.Frame(self, padding=10)
        root.pack(fill="x", expand=True)

        sliders_frame = ttk.Frame(root)
        sliders_frame.pack(fill="both", expand=True)

        SLIDERS_CONFIG = [
            {"id": "1", "title": "Kanał", "min_val": 0.0, "max_val": 10.0, "default": 0.0},
            {"id": "2", "title": "Kanał", "min_val": 0.0, "max_val": 12.0, "default": 0.0},
           # {"id": "3", "title": "Kanał", "min_val": 0.0, "max_val": 12.0, "default": 0.0},
           # {"id": "4", "title": "Kanał", "min_val": 0.0, "max_val": 12.0, "default": 0.0},
           # {"id": "5", "title": "Kanał", "min_val": 0.0, "max_val": 12.0, "default": 0.0},
           # {"id": "6", "title": "Kanał", "min_val": 0.0, "max_val": 12.0, "default": 0.0},
        ]

        for cfg in SLIDERS_CONFIG:
            block = SliderBlock(sliders_frame, cfg, on_value_change=self.on_slider_value_change)
            block.pack(fill="x", pady=5)
            self.blocks[cfg["id"]] = block

        ttk.Separator(root).pack(fill="x", pady=(1,10)) # <- ------------

        # HITy
        hits_frame = ttk.Frame(root)
        hits_frame.pack(fill="both")

        hits_frame.columnconfigure(0, weight=1)
        hits_frame.columnconfigure(1, weight=0) # podzial na grid żeby ładniej ułożyć 
        hits_frame.columnconfigure(2, weight=1)

        btns = ttk.Frame(hits_frame)
        btns.grid(row=0, column=1)

        ttk.Button(btns, text="HIT 1", width=12, command=lambda: self.on_hit_pressed(1)).pack(side="left", padx=10)
        ttk.Button(btns, text="HIT 2", width=12, command=lambda: self.on_hit_pressed(2)).pack(side="left", padx=10)

        ttk.Separator(root).pack(fill="x", pady=10) # <- ------------
        
        #Freq 
        freq_row = ttk.Frame(root)
        freq_row.pack(fill = "x")

        ttk.Checkbutton(freq_row, variable=self.freq_on,  command=self.on_freq_toggle).pack(side = "left")
        ttk.Label(freq_row, text="Frequency:").pack(side = "left", padx=1)

        freq_box = ttk.Entry(freq_row, textvariable=self.freq_val, width=12)
        freq_box.pack(side = "left")
        ttk.Label(freq_row, text = "Hz").pack(side = "left",padx=1)
        self.toggable_wid.append(freq_box)

        #HIT_Delay
        delay_row = ttk.Frame(root)
        delay_row.pack(fill = "x", pady = (10,0))

        delay_row.columnconfigure(0, weight=1)
        delay_row.columnconfigure(1, weight=0)
        delay_row.columnconfigure(2, weight=0)  # podzial na grid żeby ładniej ułożyć 
        delay_row.columnconfigure(3, weight=0)
        delay_row.columnconfigure(4, weight=1)

        delay_btn = ttk.Button(delay_row, text = "HIT1->2",command=lambda: self.on_hit_pressed(1))
        delay_btn.grid(row=0, column=1, sticky="ew", padx=(0, 10))
        ttk.Label(delay_row, text="Delay:").grid(row=0, column=2, sticky="w")

        delay_box = ttk.Entry(delay_row, textvariable=self.delay_var,width=12)
        delay_box.grid(row=0, column=3, sticky="w", padx=(1, 0))
        ttk.Label(delay_row, text="Hz").grid(row=0, column=4, sticky="w")
        self.toggable_wid.append(delay_box)
        self.toggable_wid.append(delay_btn)
        
        ttk.Separator(root).pack(fill="x", pady=10) # <- ------------
        ttk.Label(root, textvariable=self.status_var).pack(anchor="w")

    # UART
    def try_connect_uart(self):
        try:
            self.uart = STM32UART(self.uart_port, self.uart_baud)
            self.uart.open()
            self.status_var.set(f"UART: połączono ({self.uart_port})")
        except Exception as e:
            self.status_var.set(f"UART: brak połączenia ({e})")

    def uart_receive(self):
        if self.uart and self.uart.is_open():
            try:
                msg = self.uart.read_line()
                if msg:
                    self.status_var.set(f"STM: {msg}")
            except Exception as e:
                print("UART read error", e)
        self.after(50, self.uart_receive)

    # debounce slidery
    def on_slider_value_change(self, slider_id, value):
        slider_id = str(slider_id)
        self.last_value[slider_id] = float(value)

        old = self.debounce_id.get(slider_id)
        if old is not None:
            self.after_cancel(old)

        self.debounce_id[slider_id] = self.after(self.debounce_ms, lambda sid=slider_id: self.send_slider_now(sid))
        self.status_var.set(f"Slider {slider_id}: {float(value):.1f} (debounce...)")

    def send_slider_now(self, slider_id):
        self.debounce_id.pop(slider_id, None)

        value = self.last_value.get(slider_id)
        if value is None:
            return

        if not (self.uart and self.uart.is_open()):
            self.status_var.set(f"Slider {slider_id}: {value:.1f} (UART rozłączony)")
            return

        try:
            self.uart.send_line(f"{slider_id};{value:.1f}")
            self.status_var.set(f"Wysłano: {slider_id};{value:.1f}")
        except Exception as e:
            self.status_var.set(f"Błąd UART (slider): {e}")

    def on_freq_toggle(self):
        enabled = bool(self.freq_on.get())
        state = "normal" if enabled else "disabled"

        self.freq_val.set("0")
        self.delay_var.set("0")

        for w in self.toggable_wid:
            try:
                w.configure(state=state)
            except tk.TclError:
                pass
    
    # jeszcze sender do hitow
    def on_hit_pressed(self, hit_id):
        try:
            freq = self.is_good(self.freq_val.get(), "Frequency")
            delay = self.is_good(self.delay_var.get(), "Delay")
        except ValueError as e:
            self.status_var.set(str(e))
            return

        if not (self.uart and self.uart.is_open()):
            self.status_var.set(f"HIT {hit_id} (UART rozłączony)")
            return

        #H;<id>;<freq>;<delay>
        msg = f"H;{int(hit_id)};{delay:.2f};{freq:.2f}"
        try:
            self.uart.send_line(msg)
            self.status_var.set(f"Wysłano: {msg}")
        except Exception as e:
            self.status_var.set(f"Błąd UART (hit): {e}")

    def is_good(self, text, name):
        s = (text or "").strip().replace(",", ".")
        try:
            val = float(s)
        except ValueError:
            raise ValueError(f"{name} musi być liczbą")
        if val < 0:
            raise ValueError(f"{name} musi być dodatnie")
        return val

if __name__ == "__main__":
    App().mainloop()