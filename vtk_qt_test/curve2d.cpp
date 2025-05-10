#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <vtkSmartPointer.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkLine.h>
#include <vtkTextActor.h>
#include <vtkTextProperty.h>
#include <vtkCoordinate.h>
#include <QVTKOpenGLNativeWidget.h>
#include <vtkProperty.h>
#include <vtkCamera.h>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 创建主窗口
    QWidget mainWindow;
    mainWindow.setWindowTitle("VTK in Qt - 2D Sine Function");

    // 创建 vtkGenericOpenGLRenderWindow
    vtkSmartPointer<vtkGenericOpenGLRenderWindow> renderWindow =
        vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();

    // 创建 QVTKOpenGLNativeWidget 并设置渲染窗口
    QVTKOpenGLNativeWidget* vtkWidget = new QVTKOpenGLNativeWidget(&mainWindow);
    vtkWidget->setRenderWindow(renderWindow);

    // 创建渲染器并添加到渲染窗口
    vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
    renderer->SetBackground(1.0, 1.0, 1.0); // 设置背景为白色
    renderer->SetViewport(0.0, 0.0, 1.0, 1.0); // 设置视口为整个窗口
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
        points->InsertNextPoint(t, y, 0.0); // z 坐标为 0，确保在2D平面上
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
    mapper->SetScalarVisibility(false); // 禁用标量数据的使用

    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->SetColor(0.0, 0.0, 1.0); // 设置曲线颜色为蓝色

    // 添加演员到渲染器
    // 添加坐标轴
    // X轴
    vtkSmartPointer<vtkLine> xAxis = vtkSmartPointer<vtkLine>::New();
    xAxis->GetPointIds()->SetId(0, 0);
    xAxis->GetPointIds()->SetId(1, numPoints - 1);
    vtkSmartPointer<vtkCellArray> xAxisLines = vtkSmartPointer<vtkCellArray>::New();
    xAxisLines->InsertNextCell(xAxis);
    vtkSmartPointer<vtkPolyData> xAxisPolyData = vtkSmartPointer<vtkPolyData>::New();
    xAxisPolyData->SetPoints(points);
    xAxisPolyData->SetLines(xAxisLines);
    vtkSmartPointer<vtkPolyDataMapper> xAxisMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    xAxisMapper->SetInputData(xAxisPolyData);
    vtkSmartPointer<vtkActor> xAxisActor = vtkSmartPointer<vtkActor>::New();
    xAxisActor->SetMapper(xAxisMapper);
    xAxisActor->GetProperty()->SetColor(1.0, 0.0, 0.0); // 设置X轴颜色为红色
    renderer->AddActor(xAxisActor);

    // Y轴
    vtkSmartPointer<vtkPoints> yAxisPoints = vtkSmartPointer<vtkPoints>::New();
    yAxisPoints->InsertNextPoint(0.0, -1.0, 0.0);
    yAxisPoints->InsertNextPoint(0.0, 1.0, 0.0);
    vtkSmartPointer<vtkLine> yAxis = vtkSmartPointer<vtkLine>::New();
    yAxis->GetPointIds()->SetId(0, 0);
    yAxis->GetPointIds()->SetId(1, 1);
    vtkSmartPointer<vtkCellArray> yAxisLines = vtkSmartPointer<vtkCellArray>::New();
    yAxisLines->InsertNextCell(yAxis);
    vtkSmartPointer<vtkPolyData> yAxisPolyData = vtkSmartPointer<vtkPolyData>::New();
    yAxisPolyData->SetPoints(yAxisPoints);
    yAxisPolyData->SetLines(yAxisLines);
    vtkSmartPointer<vtkPolyDataMapper> yAxisMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    yAxisMapper->SetInputData(yAxisPolyData);
    vtkSmartPointer<vtkActor> yAxisActor = vtkSmartPointer<vtkActor>::New();
    yAxisActor->SetMapper(yAxisMapper);
    yAxisActor->GetProperty()->SetColor(0.0, 1.0, 0.0); // 设置Y轴颜色为绿色
    renderer->AddActor(yAxisActor);
    yAxisActor->GetProperty()->SetColor(0.0, 1.0, 0.0); // 设置Y轴颜色为绿色
    renderer->AddActor(yAxisActor);

    // 添加文本标签
    vtkSmartPointer<vtkTextActor> xLabel = vtkSmartPointer<vtkTextActor>::New();
    xLabel->SetInput("X");
    xLabel->GetTextProperty()->SetColor(1.0, 0.0, 0.0);
    xLabel->GetTextProperty()->SetFontFamilyToArial();
    xLabel->GetTextProperty()->SetFontSize(18);
    xLabel->SetDisplayPosition(10, 20);
    renderer->AddActor(xLabel);

    vtkSmartPointer<vtkTextActor> yLabel = vtkSmartPointer<vtkTextActor>::New();
    yLabel->SetInput("Y");
    yLabel->GetTextProperty()->SetColor(0.0, 1.0, 0.0);
    yLabel->GetTextProperty()->SetFontFamilyToArial();
    yLabel->GetTextProperty()->SetFontSize(18);
    yLabel->SetDisplayPosition(20, 580);
    renderer->AddActor(yLabel);

    // 设置渲染器属性
    renderer->ResetCamera();
    renderer->GetActiveCamera()->SetParallelProjection(true); // 设置为平行投影，确保2D效果

    // 设置布局
    QVBoxLayout* layout = new QVBoxLayout(&mainWindow);
    layout->addWidget(vtkWidget);
    mainWindow.setLayout(layout);

    // 显示主窗口
    mainWindow.show();

    return a.exec();
}