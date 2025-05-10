

#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScreen>
#include <QGuiApplication>
#include <vtkSmartPointer.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkSphereSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <QVTKOpenGLNativeWidget.h>
#include <vtkFloatArray.h>
#include <vtkPointData.h>
#include <vtkScalarBarActor.h>
#include <vtkPlane.h>
#include <vtkCutter.h>
#include <vtkPlaneWidget.h>
#include <vtkCommand.h>

// 回调函数，用于更新切割平面
class PlaneCallback : public vtkCommand
{
public:
    static PlaneCallback *New()
    {
        return new PlaneCallback;
    }

    void Execute(vtkObject *caller, unsigned long, void*) override
    {
        vtkPlaneWidget *planeWidget = reinterpret_cast<vtkPlaneWidget*>(caller);
        vtkPlane *plane = vtkPlane::New();
        planeWidget->GetPlane(plane);
        cutter->SetCutFunction(plane);
        cutter->Update();
        plane->Delete();
    }

    vtkCutter *cutter;
};

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 创建主窗口
    QWidget mainWindow;
    mainWindow.setWindowTitle("VTK in Qt");

    // 获取屏幕尺寸
    QScreen *screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();

    int x = 200;
    int y = 200;
    int width = 1200;
    int height = 800;

    // 确保窗口在屏幕范围内
    if (x + width > screenGeometry.width()) {
        x = screenGeometry.width() - width;
    }
    if (y + height > screenGeometry.height()) {
        y = screenGeometry.height() - height;
    }

    mainWindow.setGeometry(x, y, width, height);

    // 创建第一个 vtkGenericOpenGLRenderWindow
    vtkSmartPointer<vtkGenericOpenGLRenderWindow> renderWindow1 =
        vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();

    // 创建第一个 QVTKOpenGLNativeWidget 并设置渲染窗口
    QVTKOpenGLNativeWidget* vtkWidget1 = new QVTKOpenGLNativeWidget(&mainWindow);
    vtkWidget1->setRenderWindow(renderWindow1);

    // 创建第一个渲染器并添加到渲染窗口
    vtkSmartPointer<vtkRenderer> renderer1 = vtkSmartPointer<vtkRenderer>::New();
    renderWindow1->AddRenderer(renderer1);
    renderer1->SetBackground(0.8, 0.8, 0.8); // 设置背景颜色

    // 创建一个简单的 VTK 物体（例如一个球体）
    vtkSmartPointer<vtkSphereSource> sphereSource = vtkSmartPointer<vtkSphereSource>::New();
    sphereSource->Update();

    // 生成场数据
    vtkSmartPointer<vtkFloatArray> scalars = vtkSmartPointer<vtkFloatArray>::New();
    scalars->SetName("Scalars");
    scalars->SetNumberOfComponents(1);
    scalars->SetNumberOfTuples(sphereSource->GetOutput()->GetNumberOfPoints());

    for (vtkIdType i = 0; i < sphereSource->GetOutput()->GetNumberOfPoints(); i++)
    {
        double point[3];
        sphereSource->GetOutput()->GetPoint(i, point);
        scalars->SetValue(i, point[0]);
    }

    // 将场数据添加到球体数据集中
    sphereSource->GetOutput()->GetPointData()->SetScalars(scalars);

    // 创建映射器并设置标量可见
    vtkSmartPointer<vtkPolyDataMapper> sphereMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    sphereMapper->SetInputConnection(sphereSource->GetOutputPort());
    sphereMapper->SetScalarVisibility(1);
    sphereMapper->SetScalarModeToUsePointData();

    // 创建演员
    vtkSmartPointer<vtkActor> sphereActor = vtkSmartPointer<vtkActor>::New();
    sphereActor->SetMapper(sphereMapper);

    // 将物体添加到第一个渲染器
    renderer1->AddActor(sphereActor);

    // 创建标量条演员
    vtkSmartPointer<vtkScalarBarActor> scalarBar = vtkSmartPointer<vtkScalarBarActor>::New();
    scalarBar->SetLookupTable(sphereMapper->GetLookupTable());
    scalarBar->SetTitle("Scalars");

    // 将标量条演员添加到第一个渲染器
    renderer1->AddActor2D(scalarBar);

    // 创建切平面
    vtkSmartPointer<vtkPlane> plane = vtkSmartPointer<vtkPlane>::New();
    plane->SetOrigin(0.0, 0.0, 0.0); // 过球心
    plane->SetNormal(0.0, 1.0, 0.0); // 平面法向量

    // 创建切割器
    vtkSmartPointer<vtkCutter> cutter = vtkSmartPointer<vtkCutter>::New();
    cutter->SetCutFunction(plane);
    cutter->SetInputConnection(sphereSource->GetOutputPort());
    cutter->Update();

    // 创建第二个 vtkGenericOpenGLRenderWindow
    vtkSmartPointer<vtkGenericOpenGLRenderWindow> renderWindow2 =
        vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();

    // 创建第二个 QVTKOpenGLNativeWidget 并设置渲染窗口
    QVTKOpenGLNativeWidget* vtkWidget2 = new QVTKOpenGLNativeWidget(&mainWindow);
    vtkWidget2->setRenderWindow(renderWindow2);

    // 创建第二个渲染器并添加到渲染窗口
    vtkSmartPointer<vtkRenderer> renderer2 = vtkSmartPointer<vtkRenderer>::New();
    renderWindow2->AddRenderer(renderer2);  
    renderer2->SetBackground(0.9, 0.9, 0.9); // 设置背景颜色

    // 创建切割平面的映射器
    vtkSmartPointer<vtkPolyDataMapper> planeMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    planeMapper->SetInputConnection(cutter->GetOutputPort());
    planeMapper->SetScalarVisibility(1);
    planeMapper->SetScalarModeToUsePointData();

    // 创建切割平面的演员
    vtkSmartPointer<vtkActor> planeActor = vtkSmartPointer<vtkActor>::New();
    planeActor->SetMapper(planeMapper);

    // 将切割平面添加到第二个渲染器
    renderer2->AddActor(planeActor);

    // 创建 vtkPlaneWidget
    vtkSmartPointer<vtkPlaneWidget> planeWidget = vtkSmartPointer<vtkPlaneWidget>::New();
    planeWidget->SetInteractor(renderWindow1->GetInteractor()); // 绑定交互器
    planeWidget->SetPlaceFactor(1.25); // 设置放置因子
    planeWidget->PlaceWidget(); // 放置平面
    planeWidget->SetOrigin(plane->GetOrigin()); // 设置原点
    planeWidget->SetNormal(plane->GetNormal()); // 设置法向量
    planeWidget->SetEnabled(1);

    // 创建回调函数
    vtkSmartPointer<PlaneCallback> callback = vtkSmartPointer<PlaneCallback>::New();
    callback->cutter = cutter;
    planeWidget->AddObserver(vtkCommand::InteractionEvent, callback);

    // 设置布局
    QHBoxLayout* layout = new QHBoxLayout(&mainWindow);
    layout->addWidget(vtkWidget1);
    layout->addWidget(vtkWidget2);
    mainWindow.setLayout(layout);

    // 显示主窗口
    mainWindow.show();

    return a.exec();
}