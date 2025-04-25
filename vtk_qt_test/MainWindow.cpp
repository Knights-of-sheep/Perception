#include "MainWindow.h"
#include <vtkConeSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    
    	//圆锥模型
	vtkSmartPointer<vtkConeSource> cone = vtkSmartPointer<vtkConeSource>::New();
	cone->SetHeight(3.0);
	cone->SetRadius(1.0);
	cone->SetResolution(10);

	//映射器
	vtkSmartPointer<vtkPolyDataMapper> coneMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	coneMapper->SetInputConnection(cone->GetOutputPort());

	//对象
	vtkSmartPointer<vtkActor> coneActor = vtkSmartPointer<vtkActor>::New();
	coneActor->SetMapper(coneMapper);

	m_renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
    
    m_vtkWidget = new QVTKOpenGLNativeWidget();
    m_vtkWidget->setRenderWindow(m_renderWindow);
    m_vtkWidget->resize(800, 600);

	m_renderer = vtkSmartPointer<vtkRenderer>::New();
	m_renderer->AddActor(coneActor);
	m_renderer->SetBackground(1.0, 0.8, 0.7);
	// vtkSmartPointer<vtkRenderWindow> renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
	
	m_renderInteractor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
	m_renderInteractor->SetRenderWindow(m_renderWindow);

	m_renderWindow->AddRenderer(m_renderer);


	// 交互风格
	m_style = vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
	m_renderInteractor->SetInteractorStyle(m_style);

	m_renderInteractor->Initialize();
	m_renderInteractor->Start();
    m_vtkWidget->show();
    setCentralWidget(m_vtkWidget);
}

MainWindow::~MainWindow()
{
}

