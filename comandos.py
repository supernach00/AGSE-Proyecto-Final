import tkinter as tk
from tkinter import messagebox
import serial
import serial.tools.list_ports


# ============================================================
# Comunicación con el STM32
# ============================================================

class STM32:

    def __init__(self):
        self.ser = None

    def conectar(self, puerto):
        try:
            self.ser = serial.Serial(
                puerto,
                115200,
                timeout=1
            )

            return True

        except serial.SerialException as e:
            self.ser = None

            messagebox.showerror(
                "Error de conexión",
                f"No se pudo conectar al STM32:\n\n{e}"
            )

            return False

    def desconectar(self):

        if self.ser is not None and self.ser.is_open:
            self.ser.close()

        self.ser = None

    def enviar(self, comando):

        if self.ser is None or not self.ser.is_open:
            messagebox.showwarning(
                "Sin conexión",
                "El STM32 no está conectado."
            )
            return False

        mensaje = comando + "\n"

        try:

            self.ser.write(mensaje.encode())

            print("TX:", mensaje.strip())

            return True

        except serial.SerialException as e:

            messagebox.showerror(
                "Error USB",
                f"No se pudo enviar el comando al STM32:\n\n{e}"
            )

            self.desconectar()

            return False


# ============================================================
# Aplicación
# ============================================================

class Aplicacion:

    def __init__(self, root):

        self.root = root

        self.root.title("Generador de señales")

        self.root.geometry("450x450")

        self.stm32 = STM32()


        # ====================================================
        # TÍTULO
        # ====================================================

        titulo = tk.Label(
            root,
            text="Generador de señales",
            font=("Arial", 20)
        )

        titulo.pack(pady=15)


        # ====================================================
        # PUERTO USB
        # ====================================================

        frame_puerto = tk.Frame(root)

        frame_puerto.pack(pady=5)

        tk.Label(
            frame_puerto,
            text="Puerto USB:"
        ).pack(side="left", padx=5)


        self.puerto_var = tk.StringVar()

        self.puertos = self.obtener_puertos()


        if self.puertos:
            self.puerto_var.set(self.puertos[0])
        else:
            self.puerto_var.set("No hay puertos")


        self.menu_puertos = tk.OptionMenu(
            frame_puerto,
            self.puerto_var,
            *self.puertos
        )

        self.menu_puertos.pack(side="left")


        # ====================================================
        # BOTÓN CONECTAR
        # ====================================================

        self.boton_conectar = tk.Button(
            root,
            text="Conectar",
            width=15,
            command=self.conectar
        )

        self.boton_conectar.pack(pady=5)


        # ====================================================
        # ESTADO
        # ====================================================

        self.estado = tk.Label(
            root,
            text="Desconectado",
            fg="red"
        )

        self.estado.pack(pady=5)


        # ====================================================
        # FRECUENCIA
        # ====================================================

        frame_frecuencia = tk.Frame(root)

        frame_frecuencia.pack(pady=15)


        tk.Label(
            frame_frecuencia,
            text="Frecuencia (Hz):"
        ).pack(side="left", padx=5)


        self.frecuencia = tk.Entry(
            frame_frecuencia,
            width=12
        )

        self.frecuencia.insert(0, "1000")

        self.frecuencia.pack(side="left", padx=5)


        self.boton_frecuencia = tk.Button(
            root,
            text="Aplicar frecuencia",
            command=self.actualizar_frecuencia
        )

        self.boton_frecuencia.pack(pady=5)


        # ====================================================
        # AMPLITUD
        # ====================================================

        tk.Label(
            root,
            text="Amplitud (%)"
        ).pack(pady=(15, 0))


        self.amplitud = tk.Scale(
            root,
            from_=0,
            to=100,
            orient="horizontal",
            length=300
        )

        self.amplitud.set(50)

        self.amplitud.pack()


        # ====================================================
        # BOTÓN ACTUALIZAR AMPLITUD
        # ====================================================

        self.boton_amplitud = tk.Button(
            root,
            text="Actualizar amplitud",
            command=self.actualizar_amplitud
        )

        self.boton_amplitud.pack(pady=5)


        # ====================================================
        # ON / OFF
        # ====================================================

        frame_salida = tk.Frame(root)

        frame_salida.pack(pady=20)


        self.boton_on = tk.Button(
            frame_salida,
            text="ON",
            width=12,
            command=self.encender
        )

        self.boton_on.pack(side="left", padx=10)


        self.boton_off = tk.Button(
            frame_salida,
            text="OFF",
            width=12,
            command=self.apagar
        )

        self.boton_off.pack(side="left", padx=10)


        # ====================================================
        # CIERRE DE LA VENTANA
        # ====================================================

        self.root.protocol(
            "WM_DELETE_WINDOW",
            self.cerrar
        )


    # ========================================================
    # OBTENER PUERTOS SERIE
    # ========================================================

    def obtener_puertos(self):

        puertos = serial.tools.list_ports.comports()

        return [puerto.device for puerto in puertos]


    # ========================================================
    # CONECTAR
    # ========================================================

    def conectar(self):

        puerto = self.puerto_var.get()

        if puerto == "No hay puertos":

            messagebox.showwarning(
                "Sin puerto",
                "No hay ningún puerto serie disponible."
            )

            return


        if self.stm32.conectar(puerto):

            self.estado.config(
                text=f"Conectado: {puerto}",
                fg="green"
            )

            print("Conectado a:", puerto)


    # ========================================================
    # ACTUALIZAR FRECUENCIA
    # ========================================================

    def actualizar_frecuencia(self):

        try:

            frecuencia = float(
                self.frecuencia.get()
            )

        except ValueError:

            messagebox.showwarning(
                "Frecuencia inválida",
                "Ingresá un número válido."
            )

            return


        if frecuencia <= 0:

            messagebox.showwarning(
                "Frecuencia inválida",
                "La frecuencia debe ser mayor que cero."
            )

            return


        self.stm32.enviar(
            f"FREQ:{frecuencia}"
        )


    # ========================================================
    # ACTUALIZAR AMPLITUD
    # ========================================================

    def actualizar_amplitud(self):

        amplitud = int(
            self.amplitud.get()
        )


        self.stm32.enviar(
            f"AMP:{amplitud}"
        )


    # ========================================================
    # ENCENDER
    # ========================================================

    def encender(self):

        self.stm32.enviar(
            "ON"
        )


    # ========================================================
    # APAGAR
    # ========================================================

    def apagar(self):

        self.stm32.enviar(
            "OFF"
        )


    # ========================================================
    # CERRAR
    # ========================================================

    def cerrar(self):

        self.stm32.desconectar()

        self.root.destroy()


# ============================================================
# PROGRAMA PRINCIPAL
# ============================================================

if __name__ == "__main__":

    root = tk.Tk()

    app = Aplicacion(root)

    root.mainloop()
