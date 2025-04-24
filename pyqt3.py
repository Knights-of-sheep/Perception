from PyQt5.QtWidgets import QApplication, QMainWindow
from qtconsole.rich_ipython_widget import RichIPythonWidget
from qtconsole.inprocess import QtInProcessKernelManager

class IPythonTerminal(QMainWindow):
    def __init__(self):
        super().__init__()
        self.initUI()

    def initUI(self):
        self.terminal = RichIPythonWidget()
        self.setCentralWidget(self.terminal)
        self.kernel_manager = QtInProcessKernelManager()
        self.kernel_manager.start_kernel()
        self.kernel_client = self.kernel_manager.client()
        self.kernel_client.start_channels()
        self.terminal.kernel_manager = self.kernel_manager
        self.terminal.kernel_client = self.kernel_client

if __name__ == "__main__":
    app = QApplication([])
    window = IPythonTerminal()
    window.show()
    app.exec_()