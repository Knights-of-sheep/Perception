import sys
import subprocess
from PyQt5.QtWidgets import QApplication, QMainWindow, QTextEdit, QLineEdit, QVBoxLayout, QWidget
from PyQt5.QtCore import Qt

class Terminal(QMainWindow):
    def __init__(self):
        super().__init__()
        self.initUI()

    def initUI(self):
        self.setWindowTitle("Terminal")
        self.setGeometry(100, 100, 600, 400)

        # 创建中心窗口
        self.central_widget = QWidget()
        self.setCentralWidget(self.central_widget)

        # 创建布局
        self.layout = QVBoxLayout()
        self.central_widget.setLayout(self.layout)

        # 创建文本显示区域
        self.output_area = QTextEdit()
        self.output_area.setReadOnly(True)
        self.layout.addWidget(self.output_area)

        # 创建输入框
        self.input_line = QLineEdit()
        self.input_line.setPlaceholderText("Enter command here...")
        self.input_line.returnPressed.connect(self.execute_command)
        self.layout.addWidget(self.input_line)

    def execute_command(self):
        command = self.input_line.text()
        self.input_line.clear()
        self.output_area.append(f"> {command}")

        try:
            # 使用 subprocess 执行命令
            result = subprocess.run(command, shell=True, text=True, capture_output=True)
            self.output_area.append(result.stdout)
            if result.stderr:
                self.output_area.append(f"Error: {result.stderr}")
        except Exception as e:
            self.output_area.append(f"Error: {e}")

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = Terminal()
    window.show()
    sys.exit(app.exec_())