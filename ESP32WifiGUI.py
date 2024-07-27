import tkinter as tk
from tkinter import scrolledtext
import requests

# Replace with your ESP32 IP address and port
ESP32_IP = "172.20.10.5:8000"
SPEED_STEP = 5  # Amount by which speed changes when using arrow keys

class ESP32GUI:
    def __init__(self, root):
        self.root = root
        self.root.title("ESP32 Communication")

        # Create a Label to display the title
        self.wifi_status_label = tk.Label(root, text="Connection Status", anchor='center')
        self.wifi_status_label.grid(row=0, column=0, padx=10, pady=10)
        self.wifi_status = tk.Label(root, text="peepee...", anchor='center')
        self.wifi_status.grid(row=1, column=0, padx=10, pady=10)

        self.tcrt_status_label = tk.Label(root, text="Connection Status", anchor='center')
        self.tcrt_status_label.grid(row=0, column=2, padx=10, pady=10)
        self.tcrt_status = tk.Label(root, text="peepee...", anchor='center')
        self.tcrt_status.grid(row=1, column=2, padx=10, pady=10)

        # Create a ScrolledText widget for the output display
        self.output_box_label = tk.Label(root, text="ESP32 Communication")
        self.output_box_label.grid(row=0, column=1, padx=10, pady=10)
        self.output_box = scrolledtext.ScrolledText(root, wrap=tk.WORD, width=50, height=15)
        self.output_box.grid(row=1, column=1, padx=10, pady=10, sticky="nsew")

        # Create an Entry widget for data input
        self.entry = tk.Entry(root, width=40)
        self.entry.grid(row=3, column=0, padx=10, pady=10, sticky="ew")

        # Create a Button to send data
        self.send_button = tk.Button(root, text="Send", command=self.send_data)
        self.send_button.grid(row=3, column=1, padx=10, pady=10)

        self.entry.bind("<Return>", lambda event: self.send_data())

        self.speed_sliderA = tk.Scale(root, from_=-255, to=255, orient=tk.HORIZONTAL, label="Default Speed", command=self.update_speedA)
        self.speed_sliderA.grid(row=4, column=0, columnspan=2, padx=10, pady=10, sticky="ew")

        self.speed_sliderB = tk.Scale(root, from_=-255, to=255, orient=tk.HORIZONTAL, label="Motor SpeedB", command=self.update_speedB)
        self.speed_sliderB.grid(row=5, column=0, columnspan=2, padx=10, pady=10, sticky="ew")

        #self.root.bind("<Up>", self.increase_speed)
        #self.root.bind("<Down>", self.decrease_speed)
        #self.root.bind("<Left>", self.steer_left)
        #self.root.bind("<Right>", self.steer_right)

        self.update_display()

        # Initial speeds for motors
        self.speedA = 0
        self.speedB = 0

        root.grid_rowconfigure(0, weight=1)
        root.grid_rowconfigure(1, weight=4)
        root.grid_rowconfigure(3, weight=0)
        root.grid_columnconfigure(0, weight=1)
        root.grid_columnconfigure(1, weight=2)

    def send_data(self):
        # Split the input into endpoint and data
        input_text = self.entry.get()
        parts = input_text.split(' ', 1)
        if len(parts) == 2:
            endpoint, message = parts
        else:
            endpoint = parts[0]
            message = ""

        url = f"http://{ESP32_IP}{endpoint}"
        data = {'message': message}
        try:
            if "get" in endpoint:
                response = requests.get(url, data=data)
            else:
                response = requests.post(url, data=data)

            self.output_box.insert(tk.END, f"Sent to {url} with message {message}\n")
            self.output_box.insert(tk.END, f"Response: {response.status_code} {response.reason}\n")
            self.output_box.insert(tk.END, f"Message: {response.text}\n")
            self.output_box.insert(tk.END, "\n")
        except Exception as e:
            self.output_box.insert(tk.END, f"Error: {e}\n")
            self.output_box.insert(tk.END, "\n")
        self.entry.delete(0, tk.END)

    def update_speedA(self, value):
        url = f"http://{ESP32_IP}/defaultspeed"
        data = {'message': value}
        try:
            response = requests.post(url, data=data)
            self.output_box.insert(tk.END, f"Set motor speed to {value}\n")
            self.output_box.insert(tk.END, f"Response: {response.status_code} {response.reason}\n")
            self.output_box.insert(tk.END, f"Message: {response.text}\n")
            self.output_box.insert(tk.END, "\n")
            self.output_box.see(tk.END)
        except Exception as e:
            self.output_box.insert(tk.END, f"Error: {e}\n")
            self.output_box.insert(tk.END, "\n")
            self.output_box.see(tk.END)

    def update_speedB(self, value):
        url = f"http://{ESP32_IP}/speedB"
        data = {'message': value}
        try:
            response = requests.post(url, data=data)
            self.output_box.insert(tk.END, f"Set motor speed to {value}\n")
            self.output_box.insert(tk.END, f"Response: {response.status_code} {response.reason}\n")
            self.output_box.insert(tk.END, f"Message: {response.text}\n")
            self.output_box.insert(tk.END, "\n")
            self.output_box.see(tk.END)
        except Exception as e:
            self.output_box.insert(tk.END, f"Error: {e}\n")
            self.output_box.insert(tk.END, "\n")
            self.output_box.see(tk.END)

    def decrease_speed(self, event):
        self.speedA = self.speed_sliderA.get() - SPEED_STEP
        self.speedB = self.speed_sliderB.get() - SPEED_STEP
        self.speed_sliderA.set(self.speedA)
        self.speed_sliderB.set(self.speedB)
        self.update_speedA(self.speedA)
        self.update_speedB(self.speedB)

    def increase_speed(self, event):
        self.speedA = self.speed_sliderA.get() + SPEED_STEP
        self.speedB = self.speed_sliderB.get() + SPEED_STEP
        self.speed_sliderA.set(self.speedA)
        self.speed_sliderB.set(self.speedB)
        self.update_speedA(self.speedA)
        self.update_speedB(self.speedB)

    def steer_left(self, event):
        self.speedA = self.speed_sliderA.get() + SPEED_STEP
        self.speedB = self.speed_sliderB.get() - SPEED_STEP
        self.speedA = min(max(self.speedA, -255), 255)  # Clamp to -255 to 255
        self.speedB = min(max(self.speedB, -255), 255)  # Clamp to -255 to 255
        self.speed_sliderA.set(self.speedA)
        self.speed_sliderB.set(self.speedB)
        self.update_speedA(self.speedA)
        self.update_speedB(self.speedB)

    def steer_right(self, event):
        self.speedA = self.speed_sliderA.get() - SPEED_STEP
        self.speedB = self.speed_sliderB.get() + SPEED_STEP
        self.speedA = min(max(self.speedA, -255), 255)  # Clamp to -255 to 255
        self.speedB = min(max(self.speedB, -255), 255)  # Clamp to -255 to 255
        self.speed_sliderA.set(self.speedA)
        self.speed_sliderB.set(self.speedB)
        self.update_speedA(self.speedA)
        self.update_speedB(self.speedB)


    def update_display(self):
        url = f"http://{ESP32_IP}/getTCRTs"
        try:
            response = requests.get(url)
            self.wifi_status.config(text=response.text)
        except Exception as e:
            self.wifi_status.config(text=f"Error: {e}\n\n")
        self.root.after(500, self.update_display)  # Update every 1 secons

if __name__ == "__main__":
    root = tk.Tk()
    app = ESP32GUI(root)
    root.mainloop()
