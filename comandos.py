import tkinter as tk
from tkinter import ttk, messagebox
import serial
import serial.tools.list_ports


# ============================================================
# COMUNICACIÓN CON EL STM32
# ============================================================

class STM32:

    def __init__(self, app):
        self.ser = None
        self.app = app

    def conectar(self, puerto):

        try:

            self.ser = serial.Serial(
                puerto,
                115200,
                timeout=1
            )

            self.app.actualizar_indicador(True)

            return True

        except serial.SerialException as e:

            self.app.actualizar_indicador(False)

            messagebox.showerror(
                "Error",
                f"No se pudo abrir el puerto:\n{e}"
            )

            return False

    def desconectar(self):

        if self.ser is not None and self.ser.is_open:
            self.ser.close()

        self.ser = None

        self.app.actualizar_indicador(False)

    def enviar(self, comando):

        if self.ser is None or not self.ser.is_open:

            self.app.actualizar_indicador(False)

            messagebox.showwarning(
                "Sin conexión",
                "Primero conectá el STM32."
            )

            return False

        try:

            mensaje = comando + "\n"

            self.ser.write(
                mensaje.encode()
            )

            print("TX:", comando)

            return True

        except serial.SerialException as e:

            self.desconectar()

            messagebox.showerror(
                "Error USB",
                f"No se pudo enviar el comando:\n{e}"
            )

            return False


# ============================================================
# APLICACIÓN
# ============================================================

class Aplicacion:

    def __init__(self, root):

        self.root = root

        self.root.title(
            "Generador de señales - STM32"
        )

        self.root.geometry(
            "700x720"
        )

        self.stm32 = STM32(self)

        # ----------------------------------------------------
        # VARIABLES
        # ----------------------------------------------------

        self.frecuencia_L = tk.StringVar(
            value="1000"
        )

        self.frecuencia_R = tk.StringVar(
            value="1000"
        )

        # Amplitud interna SIEMPRE en porcentaje

        self.amplitud_L = tk.IntVar(
            value=50
        )

        self.amplitud_R = tk.IntVar(
            value=50
        )

        # Unidad seleccionada

        self.unidad_amp_L = tk.StringVar(
            value="%"
        )

        self.unidad_amp_R = tk.StringVar(
            value="%"
        )

        self.estado_L = False
        self.estado_R = False

        # ----------------------------------------------------
        # INTERFAZ
        # ----------------------------------------------------

        self.crear_interfaz()

        self.actualizar_puertos()


    # ========================================================
    # INTERFAZ
    # ========================================================

    def crear_interfaz(self):

        # ====================================================
        # CONEXIÓN
        # ====================================================

        frame_conexion = ttk.Frame(
            self.root,
            padding=10
        )

        frame_conexion.pack(
            fill="x"
        )

        ttk.Label(
            frame_conexion,
            text="Puerto:"
        ).pack(
            side="left"
        )

        self.combo_puertos = ttk.Combobox(
            frame_conexion,
            state="readonly",
            width=15
        )

        self.combo_puertos.pack(
            side="left",
            padx=5
        )

        ttk.Button(
            frame_conexion,
            text="Actualizar",
            command=self.actualizar_puertos
        ).pack(
            side="left",
            padx=5
        )

        self.boton_conectar = ttk.Button(
            frame_conexion,
            text="Conectar",
            command=self.conectar
        )

        self.boton_conectar.pack(
            side="left",
            padx=5
        )

        # ----------------------------------------------------
        # INDICADOR
        # ----------------------------------------------------

        self.indicador = tk.Canvas(
            frame_conexion,
            width=20,
            height=20,
            highlightthickness=0
        )

        self.indicador.pack(
            side="left",
            padx=(15, 5)
        )

        self.luz = self.indicador.create_oval(
            3,
            3,
            17,
            17,
            fill="red",
            outline="black"
        )

        self.label_estado = ttk.Label(
            frame_conexion,
            text="Desconectado"
        )

        self.label_estado.pack(
            side="left"
        )

        # ====================================================
        # SEPARADOR
        # ====================================================

        ttk.Separator(
            self.root,
            orient="horizontal"
        ).pack(
            fill="x",
            padx=10,
            pady=5
        )

        # ====================================================
        # CANALES
        # ====================================================

        frame_canales = ttk.Frame(
            self.root,
            padding=10
        )

        frame_canales.pack(
            fill="both",
            expand=True
        )

        # ====================================================
        # CANAL LEFT
        # ====================================================

        frame_L = ttk.Frame(
            frame_canales,
            padding=15
        )

        frame_L.pack(
            side="left",
            fill="both",
            expand=True
        )

        ttk.Label(
            frame_L,
            text="CANAL LEFT",
            font=("Arial", 16, "bold")
        ).pack(
            pady=10
        )

        # ----------------------------------------------------
        # FRECUENCIA L
        # ----------------------------------------------------

        ttk.Label(
            frame_L,
            text="Frecuencia (Hz)"
        ).pack(
            pady=(10, 2)
        )

        self.entry_freq_L = ttk.Entry(
            frame_L,
            textvariable=self.frecuencia_L,
            width=15,
            justify="center"
        )

        self.entry_freq_L.pack()

        ttk.Button(
            frame_L,
            text="Aplicar frecuencia",
            command=self.actualizar_frecuencia_L
        ).pack(
            pady=5
        )

        # ----------------------------------------------------
        # AMPLITUD L
        # ----------------------------------------------------

        ttk.Label(
            frame_L,
            text="Amplitud"
        ).pack(
            pady=(20, 2)
        )

        # Selector de unidad

        self.combo_unidad_L = ttk.Combobox(
            frame_L,
            textvariable=self.unidad_amp_L,
            values=["%", "V"],
            state="readonly",
            width=5
        )

        self.combo_unidad_L.pack(
            pady=2
        )

        self.combo_unidad_L.bind(
            "<<ComboboxSelected>>",
            self.cambiar_unidad_L
        )

        self.label_amp_L = ttk.Label(
            frame_L,
            text="50 %"
        )

        self.label_amp_L.pack()

        # Slider siempre representa 0-100 %

        self.slider_L = ttk.Scale(
            frame_L,
            from_=0,
            to=100,
            orient="horizontal",
            command=self.cambiar_slider_L
        )

        self.slider_L.set(50)

        self.slider_L.pack(
            fill="x",
            padx=20,
            pady=5
        )

        ttk.Button(
            frame_L,
            text="Actualizar amplitud",
            command=self.actualizar_amplitud_L
        ).pack(
            pady=5
        )

        # ----------------------------------------------------
        # ON / OFF L
        # ----------------------------------------------------

        frame_botones_L = ttk.Frame(
            frame_L
        )

        frame_botones_L.pack(
            pady=30
        )

        ttk.Button(
            frame_botones_L,
            text="ON",
            command=self.encender_L
        ).pack(
            side="left",
            padx=5
        )

        ttk.Button(
            frame_botones_L,
            text="OFF",
            command=self.apagar_L
        ).pack(
            side="left",
            padx=5
        )

        # ====================================================
        # SEPARADOR VERTICAL
        # ====================================================

        ttk.Separator(
            frame_canales,
            orient="vertical"
        ).pack(
            side="left",
            fill="y",
            padx=5
        )

        # ====================================================
        # CANAL RIGHT
        # ====================================================

        frame_R = ttk.Frame(
            frame_canales,
            padding=15
        )

        frame_R.pack(
            side="left",
            fill="both",
            expand=True
        )

        ttk.Label(
            frame_R,
            text="CANAL RIGHT",
            font=("Arial", 16, "bold")
        ).pack(
            pady=10
        )

        # ----------------------------------------------------
        # FRECUENCIA R
        # ----------------------------------------------------

        ttk.Label(
            frame_R,
            text="Frecuencia (Hz)"
        ).pack(
            pady=(10, 2)
        )

        self.entry_freq_R = ttk.Entry(
            frame_R,
            textvariable=self.frecuencia_R,
            width=15,
            justify="center"
        )

        self.entry_freq_R.pack()

        ttk.Button(
            frame_R,
            text="Aplicar frecuencia",
            command=self.actualizar_frecuencia_R
        ).pack(
            pady=5
        )

        # ----------------------------------------------------
        # AMPLITUD R
        # ----------------------------------------------------

        ttk.Label(
            frame_R,
            text="Amplitud"
        ).pack(
            pady=(20, 2)
        )

        self.combo_unidad_R = ttk.Combobox(
            frame_R,
            textvariable=self.unidad_amp_R,
            values=["%", "V"],
            state="readonly",
            width=5
        )

        self.combo_unidad_R.pack(
            pady=2
        )

        self.combo_unidad_R.bind(
            "<<ComboboxSelected>>",
            self.cambiar_unidad_R
        )

        self.label_amp_R = ttk.Label(
            frame_R,
            text="50 %"
        )

        self.label_amp_R.pack()

        # Slider 0-100 %

        self.slider_R = ttk.Scale(
            frame_R,
            from_=0,
            to=100,
            orient="horizontal",
            command=self.cambiar_slider_R
        )

        self.slider_R.set(50)

        self.slider_R.pack(
            fill="x",
            padx=20,
            pady=5
        )

        ttk.Button(
            frame_R,
            text="Actualizar amplitud",
            command=self.actualizar_amplitud_R
        ).pack(
            pady=5
        )

        # ----------------------------------------------------
        # ON / OFF R
        # ----------------------------------------------------

        frame_botones_R = ttk.Frame(
            frame_R
        )

        frame_botones_R.pack(
            pady=30
        )

        ttk.Button(
            frame_botones_R,
            text="ON",
            command=self.encender_R
        ).pack(
            side="left",
            padx=5
        )

        ttk.Button(
            frame_botones_R,
            text="OFF",
            command=self.apagar_R
        ).pack(
            side="left",
            padx=5
        )

        # panel de forma de onda + modo de salida
        self.crear_extras()


    # ========================================================
    # FORMA DE ONDA Y MODO DE SALIDA
    # ========================================================

    def crear_extras(self):

        # --- forma de onda por canal ---
        cont = ttk.Frame(self.root)
        cont.pack(pady=10)

        ttk.Label(
            cont, text="Forma de onda"
        ).grid(row=0, column=0, columnspan=5, pady=(0, 4))

        formas = ["sine", "square", "tri", "chirp"]

        ttk.Label(cont, text="Izq (L)").grid(row=1, column=0, padx=4)
        for i, f in enumerate(formas):
            self.boton_onda(cont, f, "L").grid(row=1, column=1 + i, padx=3, pady=2)

        ttk.Label(cont, text="Der (R)").grid(row=2, column=0, padx=4)
        for i, f in enumerate(formas):
            self.boton_onda(cont, f, "R").grid(row=2, column=1 + i, padx=3, pady=2)

        # --- modo de salida (diferencial / simple) ---
        frame_salida = ttk.Frame(self.root)
        frame_salida.pack(pady=8)

        ttk.Label(frame_salida, text="Salida:").pack(side="left", padx=5)

        self.modo_salida = tk.StringVar(value="DIFF")

        ttk.Radiobutton(
            frame_salida, text="Diferencial", variable=self.modo_salida,
            value="DIFF", command=self.aplicar_salida
        ).pack(side="left", padx=5)

        ttk.Radiobutton(
            frame_salida, text="Simple", variable=self.modo_salida,
            value="SE", command=self.aplicar_salida
        ).pack(side="left", padx=5)


    def boton_onda(self, parent, forma, canal):

        c = tk.Canvas(
            parent, width=48, height=30, bg="white",
            highlightthickness=1, highlightbackground="#999", cursor="hand2"
        )
        self.dibujar_onda(c, forma)
        c.bind("<Button-1>", lambda e: self.set_onda(canal, forma))
        return c


    def dibujar_onda(self, c, forma):

        import math
        w, h = 48, 30
        mid, amp = h / 2, h / 2 - 4
        col = "#0B63C4"

        if forma == "sine":
            pts = []
            for x in range(2, w - 1):
                y = mid - amp * math.sin(2 * math.pi * 2 * (x - 2) / (w - 4))
                pts += [x, y]
            c.create_line(*pts, fill=col, width=2)

        elif forma == "square":
            t, b, x0 = 4, h - 4, 2
            q = (w - 4) // 4
            c.create_line(x0, t, x0 + q, t, x0 + q, b, x0 + 2 * q, b,
                          x0 + 2 * q, t, x0 + 3 * q, t, x0 + 3 * q, b,
                          x0 + 4 * q, b, fill=col, width=2)

        elif forma == "tri":
            t, b, x0 = 4, h - 4, 2
            q = (w - 4) // 4
            c.create_line(x0, b, x0 + q, t, x0 + 2 * q, b,
                          x0 + 3 * q, t, x0 + 4 * q, b, fill=col, width=2)

        elif forma == "chirp":
            pts = []
            for x in range(2, w - 1):
                tt = (x - 2) / (w - 4)
                y = mid - amp * math.sin(2 * math.pi * (1 + 5 * tt) * tt)
                pts += [x, y]
            c.create_line(*pts, fill=col, width=2)


    def set_onda(self, canal, forma):

        idx = {"sine": 0, "square": 1, "tri": 2, "chirp": 3}[forma]
        self.stm32.enviar(f"{canal}WAVE:{idx}")


    def aplicar_salida(self):

        self.stm32.enviar(self.modo_salida.get())   # manda "DIFF" o "SE"


    # ========================================================
    # INDICADOR DE CONEXIÓN
    # ========================================================

    def actualizar_indicador(self, conectado):

        if conectado:

            self.indicador.itemconfig(
                self.luz,
                fill="green"
            )

            self.label_estado.config(
                text="Conectado"
            )

            self.boton_conectar.config(
                text="Conectado",
                state="disabled"
            )

        else:

            self.indicador.itemconfig(
                self.luz,
                fill="red"
            )

            self.label_estado.config(
                text="Desconectado"
            )

            self.boton_conectar.config(
                text="Conectar",
                state="normal"
            )


    # ========================================================
    # PUERTOS
    # ========================================================

    def actualizar_puertos(self):

        puertos = serial.tools.list_ports.comports()

        nombres = [
            puerto.device
            for puerto in puertos
        ]

        self.combo_puertos["values"] = nombres

        if nombres:
            self.combo_puertos.current(0)


    # ========================================================
    # CONEXIÓN
    # ========================================================

    def conectar(self):

        puerto = self.combo_puertos.get()

        if not puerto:

            messagebox.showwarning(
                "Puerto",
                "Seleccioná un puerto."
            )

            return

        self.stm32.conectar(puerto)


    # ========================================================
    # CONVERSIÓN DE AMPLITUD
    # ========================================================

    def porcentaje_a_voltaje(self, porcentaje):

        return (
            porcentaje / 100.0
        ) * 3.3


    def voltaje_a_porcentaje(self, voltaje):

        porcentaje = (
            voltaje / 3.3
        ) * 100.0

        return porcentaje


    # ========================================================
    # CANAL LEFT
    # ========================================================

    def cambiar_slider_L(self, valor):

        porcentaje = int(
            float(valor)
        )

        self.amplitud_L.set(
            porcentaje
        )

        if self.unidad_amp_L.get() == "%":

            self.label_amp_L.config(
                text=f"{porcentaje} %"
            )

        else:

            voltaje = self.porcentaje_a_voltaje(
                porcentaje
            )

            self.label_amp_L.config(
                text=f"{voltaje:.2f} V"
            )


    def cambiar_unidad_L(self, event=None):

        porcentaje = self.amplitud_L.get()

        if self.unidad_amp_L.get() == "%":

            self.label_amp_L.config(
                text=f"{porcentaje} %"
            )

        else:

            voltaje = self.porcentaje_a_voltaje(
                porcentaje
            )

            self.label_amp_L.config(
                text=f"{voltaje:.2f} V"
            )


    def actualizar_amplitud_L(self):

        porcentaje = self.amplitud_L.get()

        self.stm32.enviar(
            f"LAMP:{porcentaje}"
        )


    def actualizar_frecuencia_L(self):

        try:

            frecuencia = float(
                self.frecuencia_L.get()
            )

            if frecuencia <= 0:
                raise ValueError

            self.stm32.enviar(
                f"LFREQ:{frecuencia}"
            )

        except ValueError:

            messagebox.showerror(
                "Frecuencia",
                "Ingresá una frecuencia válida."
            )


    def encender_L(self):

        if self.stm32.enviar("LON"):
            self.estado_L = True


    def apagar_L(self):

        if self.stm32.enviar("LOFF"):
            self.estado_L = False


    # ========================================================
    # CANAL RIGHT
    # ========================================================

    def cambiar_slider_R(self, valor):

        porcentaje = int(
            float(valor)
        )

        self.amplitud_R.set(
            porcentaje
        )

        if self.unidad_amp_R.get() == "%":

            self.label_amp_R.config(
                text=f"{porcentaje} %"
            )

        else:

            voltaje = self.porcentaje_a_voltaje(
                porcentaje
            )

            self.label_amp_R.config(
                text=f"{voltaje:.2f} V"
            )


    def cambiar_unidad_R(self, event=None):

        porcentaje = self.amplitud_R.get()

        if self.unidad_amp_R.get() == "%":

            self.label_amp_R.config(
                text=f"{porcentaje} %"
            )

        else:

            voltaje = self.porcentaje_a_voltaje(
                porcentaje
            )

            self.label_amp_R.config(
                text=f"{voltaje:.2f} V"
            )


    def actualizar_amplitud_R(self):

        porcentaje = self.amplitud_R.get()

        self.stm32.enviar(
            f"RAMP:{porcentaje}"
        )


    def actualizar_frecuencia_R(self):

        try:

            frecuencia = float(
                self.frecuencia_R.get()
            )

            if frecuencia <= 0:
                raise ValueError

            self.stm32.enviar(
                f"RFREQ:{frecuencia}"
            )

        except ValueError:

            messagebox.showerror(
                "Frecuencia",
                "Ingresá una frecuencia válida."
            )


    def encender_R(self):

        if self.stm32.enviar("RON"):
            self.estado_R = True


    def apagar_R(self):

        if self.stm32.enviar("ROFF"):
            self.estado_R = False


# ============================================================
# MAIN
# ============================================================

if __name__ == "__main__":

    root = tk.Tk()

    app = Aplicacion(root)

    root.mainloop()

