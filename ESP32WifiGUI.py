import tkinter as tk
from tkinter import scrolledtext
import requests

# Replace with your ESP32 IP address and port
ESP32_IP = "206.12.161.107:8000"

class ESP32GUI:
    def __init__(self, root):
        self.root = root
        self.root.title("ESP32 Communication")

        # Create a Label to display the title
        self.output_label = tk.Label(root, text="ESP32 Communication", anchor='center')
        self.output_label.grid(row=0, column=0, padx=10, pady=10, sticky="nsew")

        # Create a ScrolledText widget for the output display
        self.output_box = scrolledtext.ScrolledText(root, wrap=tk.WORD, width=50, height=15)
        self.output_box.grid(row=0, column=1, padx=10, pady=10, sticky="nsew")

        # Create an Entry widget for data input
        self.entry = tk.Entry(root, width=40)
        self.entry.grid(row=1, column=0, padx=10, pady=10, sticky="ew")

        # Create a Button to send data
        self.send_button = tk.Button(root, text="Send", command=self.send_data)
        self.send_button.grid(row=1, column=1, padx=10, pady=10)

        self.entry.bind("<Return>", self.send_data)

        # Configure grid weights
        root.grid_columnconfigure(0, weight=1)  # Column 0 has weight 1
        root.grid_columnconfigure(1, weight=100)  # Column 1 has weight 2
        root.grid_rowconfigure(1, weight=1)

        # Start the display update loop
        self.update_display()

        root.grid_columnconfigure(0, weight=2)
        root.grid_columnconfigure(1, weight=2)
        root.grid_rowconfigure(1, weight=1)

    def send_data(self, event=None):
        message = self.entry.get()
        url = f"http://{ESP32_IP}/post"
        data = {'message': message}
        try:
            response = requests.post(url, data=data)
            self.output_box.insert(tk.END, f"Sent: {message}\n")
            self.output_box.insert(tk.END, f"Response: {response.status_code} {response.reason}\n")
            self.output_box.insert(tk.END, f"Message: {response.text}\n")
        except Exception as e:
            self.output_box.insert(tk.END, f"Error: {e}\n")
        self.entry.delete(0, tk.END)

    def update_display(self):
        # Placeholder for constantly updating data
        self.output_label.config(text = "Updating display...\n")
        self.root.after(5000, self.update_display)  # Update every 5 seconds

if __name__ == "__main__":
    root = tk.Tk()
    app = ESP32GUI(root)
    root.mainloop()
