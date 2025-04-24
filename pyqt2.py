from PyQt5.QtWidgets import QApplication, QMainWindow, QVBoxLayout, QWidget, QPlainTextEdit, QLineEdit, QPushButton
from PyQt5.QtCore import QProcess

class TerminalWidget(QWidget):
    def __init__(self):
        super().__init__()
        self.initUI()

    def initUI(self):
        layout = QVBoxLayout(self)
        self.output = QPlainTextEdit(self)
        self.output.setReadOnly(True)
        self.input = QLineEdit(self)
        self.executeButton = QPushButton("Execute", self)

        layout.addWidget(self.output)
        layout.addWidget(self.input)
        layout.addWidget(self.executeButton)

        self.input.returnPressed.connect(self.executeCommand)
        self.executeButton.clicked.connect(self.executeCommand)

    def executeCommand(self):
        command = self.input.text()
        process = QProcess()
        process.start("python", ["-c", command])
        process.waitForFinished()
        result = process.readAllStandardOutput().data().decode()
        self.output.appendPlainText(result)
        self.input.clear()

if __name__ == "__main__":
    app = QApplication([])
    window = QMainWindow()
    terminal = TerminalWidget()
    window.setCentralWidget(terminal)
    window.show()
    app.exec_()