#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QScreen>
#include <QGuiApplication>
#include <vtkSmartPointer.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkLine.h>
#include <vtkAxesActor.h>
#include <vtkOrientationMarkerWidget.h>
#include <vtkTextProperty.h>
#include <QVTKOpenGLNativeWidget.h>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 创建主窗口
    QWidget mainWindow;
    mainWindow.setWindowTitle("VTK in Qt - Sine Function");

    // 获取屏幕尺寸
    QScreen *screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();

    int x = 200;
    int y = 200;
    int width = 800;
    int height = 600;

    // 确保窗口在屏幕范围内
    if (x + width > screenGeometry.width()) {
        x = screenGeometry.width() - width;
    }
    if (y + height > screenGeometry.height()) {
        y = screenGeometry.height() - height;
    }

    mainWindow.setGeometry(x, y, width, height);

    // 创建 vtkGenericOpenGLRenderWindow
    vtkSmartPointer<vtkGenericOpenGLRenderWindow> renderWindow =
        vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();

    // 创建 QVTKOpenGLNativeWidget 并设置渲染窗口
    QVTKOpenGLNativeWidget* vtkWidget = new QVTKOpenGLNativeWidget(&mainWindow);
    vtkWidget->setRenderWindow(renderWindow);

    // 创建渲染器并添加到渲染窗口
    vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
    renderWindow->AddRenderer(renderer);

    // 创建正弦函数数据
    vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New();
    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkCellArray> lines = vtkSmartPointer<vtkCellArray>::New();

    const int numPoints = 100;
    for (int i = 0; i < numPoints; ++i)
    {
        double t = static_cast<double>(i) / (numPoints - 1) * 10.0; // x 范围从 0 到 10
        double y = sin(t); // y = sin(x)
        points->InsertNextPoint(t, y, 0.0);
    }

    // 创建线
    vtkSmartPointer<vtkLine> line = vtkSmartPointer<vtkLine>::New();
    for (int i = 0; i < numPoints - 1; ++i)
    {
        line->GetPointIds()->SetId(0, i);
        line->GetPointIds()->SetId(1, i + 1);
        lines->InsertNextCell(line);
    }

    polyData->SetPoints(points);
    polyData->SetLines(lines);

    // 创建映射器和演员
    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputData(polyData);

    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);

    // 添加演员到渲染器
    renderer->AddActor(actor);

    // 添加坐标轴
    vtkSmartPointer<vtkAxesActor> axes = vtkSmartPointer<vtkAxesActor>::New();
    vtkSmartPointer<vtkOrientationMarkerWidget> markerWidget = vtkSmartPointer<vtkOrientationMarkerWidget>::New();
    markerWidget->SetOutlineColor(0.93, 0.93, 0.5);
    markerWidget->SetOrientationMarker(axes);
    markerWidget->SetInteractor(renderWindow->GetInteractor());
    markerWidget->SetViewport(0.0, 0.0, 0.2, 0.2);
    markerWidget->SetEnabled(1);
    markerWidget->InteractiveOn();

    // 设置渲染器属性
    renderer->SetBackground(0.2, 0.3, 0.4); // 设置背景颜色
    renderer->ResetCamera(); // 重置相机以适应场景

    // 设置布局
    QVBoxLayout* layout = new QVBoxLayout(&mainWindow);
    layout->addWidget(vtkWidget);
    mainWindow.setLayout(layout);

    // 显示主窗口
    mainWindow.show();

    return a.exec();
}