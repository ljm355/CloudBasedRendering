

#include <Windows.h>
#include <osgGA/TrackballManipulator>
#include <osgGA/FlightManipulator>
#include <osgGA/DriveManipulator>
#include <osgGA/KeySwitchMatrixManipulator>
#include <osgGA/StateSetManipulator>
#include <osgGA/AnimationPathManipulator>
#include <osgGA/TerrainManipulator>
#include <osgGA/SphericalManipulator>
#include <osg/GLExtensions>
#include <osgViewer/Renderer>
#include <osgGA/TrackballManipulator>
#include <osgDB/WriteFile>
#include <osgViewer/ViewerEventHandlers>
#include <osg/ClampColor>
#include <osg/MatrixTransform>
#include "osg/LineWidth"
#include "Teapot.h"
#include <math.h>
#include <QUdpSocket>
#include <QCoreApplication>
#include "CameraBuffer.h"
#include <QUdpSocket>
#include "../CloudClient/MessageType.h"
std::vector<osg::GraphicsContext*> _contexts;
osg::ref_ptr<CameraBuffer> _cameraBuffer;
QUdpSocket *_socket;
osgViewer::Viewer* _viewer;
osg::ref_ptr<osg::Group> _root;
unsigned short _clientPort = 1234;
unsigned short _serverPort = 1235;
QHostAddress _serverAddress = QHostAddress::LocalHost;
QHostAddress _clientAddress = QHostAddress::LocalHost;
bool _clientInitialized = false;
unsigned long _requested_stamp = 0;
void dispatch_BOUNDINGSPHERE()
{
	std::stringstream ms;
	ms.write(STATE_MESSAGE.data(), STATE_MESSAGE.size());
	char tp = MESSAGE_TYPE::BOUNDINGSPHERE;
	ms.write(&tp, 1);
	osg::BoundingSphere bs = _root->getBound();
	osg::Vec3 center = bs.center();
	float radius = bs.radius();
	ms.write((char*)(&center), sizeof(float) * 3);
	ms.write((char*)(&radius), sizeof(float));
	_socket->writeDatagram(ms.str().data(), ms.str().size(), _clientAddress, _clientPort);
}
void receive_VIEWMATRIX(std::stringstream& ms)
{
	ms.seekg(STATE_MESSAGE.size() + 1, ms.beg);
	osg::Matrix matview;
	ms.read((char*)matview.ptr(), sizeof(double)*16);
	ms.read((char*)&_requested_stamp, sizeof(unsigned long));
	//printf("_requested_stamp=%d\n", _requested_stamp);
	_cameraBuffer->setViewMatrix(matview);
	_viewer->getCamera()->setViewMatrix(matview);
}
void receive_PROJECTIONMATRIX(std::stringstream& ms)
{
	ms.seekg(STATE_MESSAGE.size() + 1, ms.beg);
	osg::Matrix matproj;
	ms.read((char*)matproj.ptr(), sizeof(double) * 16);
	_cameraBuffer->setProjectionMatrix(matproj);
	_viewer->getCamera()->setProjectionMatrix(matproj);
}

void receive_RESIZE(std::stringstream& ms)
{
	ms.seekg(STATE_MESSAGE.size() + 1, ms.beg);
	int width, height;
	ms.read((char*)&width, sizeof(int));
	ms.read((char*)&height, sizeof(int));
	//_cameraBuffer->setupBuffer(width, height);

	_viewer->removeSlave(0);
	_cameraBuffer = CameraBuffer::createSlave(width, height, _contexts[0]);
	_viewer->addSlave(_cameraBuffer.get(), false);
	osg::ref_ptr<osg::Geode> teapot = createTeapot();
	_cameraBuffer->addChild(teapot.get());
}

void receive_CLIENT_LOGIN(std::stringstream& ms)
{
	ms.seekg(STATE_MESSAGE.size() + 1, ms.beg);
	unsigned int clientaddress = 0;
	ms.read((char*)&clientaddress, sizeof(unsigned int));
	ms.read((char*)&_clientPort, sizeof(unsigned short));
	_clientAddress = QHostAddress(clientaddress);
	printf("client logged in from %s:%d\n", _clientAddress.toString().toLocal8Bit().data(), _clientPort);
}

int main(int argc, char **argv)
{
	if (argc > 1)
	{
		_serverAddress = QHostAddress(QString(argv[1]));
		_serverPort = atoi(argv[2]);
	}
	std::string nodefile = "";
	if (argc > 3)
	{
		nodefile = argv[3];
	}
	osg::ref_ptr<osg::Node> sceneNode = osgDB::readNodeFile(nodefile);
	if (!sceneNode || !sceneNode.valid())
	{
		sceneNode = createTeapot();
	}
	QCoreApplication qapp(argc, argv);
	_socket = new QUdpSocket;
	_socket->bind(_serverAddress, _serverPort);         //ex. Address localhost, port 1234
	printf("server launched at %s:%d\n", _serverAddress.toString().toLocal8Bit().data(), _serverPort);
	osgViewer::Viewer viewer;
	viewer.setUpViewAcrossAllScreens();
	viewer.setUpViewInWindow(50, 50, 1024, 768);
	viewer.getContexts(_contexts);
	_cameraBuffer = CameraBuffer::createSlave(1024, 768, _contexts[0]);
	viewer.addSlave(_cameraBuffer.get(), false);
	_cameraBuffer->addChild(sceneNode.get());
	_cameraBuffer->setClearColor(viewer.getCamera()->getClearColor());
	_viewer = &viewer;
	_root = new osg::Group;
	_root->addChild(sceneNode.get());
	viewer.setSceneData(_root.get());
	
	viewer.setThreadingModel(osgViewer::ViewerBase::ThreadingModel::ThreadPerContext);

	// add the state manipulator
	viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getOrCreateStateSet()));
	//viewer.setCameraManipulator(new osgGA::TrackballManipulator);
	// add the thread model handler
	viewer.addEventHandler(new osgViewer::ThreadingHandler);

	// add the window size toggle handler
	viewer.addEventHandler(new osgViewer::WindowSizeHandler);

	// add the stats handler
	viewer.addEventHandler(new osgViewer::StatsHandler);

	// add the LOD Scale handler
	viewer.addEventHandler(new osgViewer::LODScaleHandler);

	// add the screen capture handler
	viewer.addEventHandler(new osgViewer::ScreenCaptureHandler);
	unsigned long counter = 0;
	viewer.realize();
	//osgDB::Registry::instance()->loadLibrary("png"); // or "osgdb_png", not 
	osgDB::Registry::instance()->loadLibrary("jpg"); // or "osgdb_png", not 
	osg::ref_ptr<osgDB::ReaderWriter> jpgRW = osgDB::Registry::instance()->getReaderWriterForExtension("jpg");
	osgDB::ReaderWriter::ReadResult rr;

	bool requested = false;
	while (!viewer.done())
	{

		while (_socket->hasPendingDatagrams())
		{
			QByteArray Buffer;
			Buffer.resize(_socket->pendingDatagramSize());
			QHostAddress sender;
			quint16 senderPort;
			_socket->readDatagram(Buffer.data(), Buffer.size(), &_clientAddress, &_clientPort);
			std::stringstream ms;
			ms.write(Buffer.data(), Buffer.size());
			if (ms.str().substr(0, STATE_MESSAGE.size()) == STATE_MESSAGE)
			{
				char msg_type = ms.str().data()[STATE_MESSAGE.size()];
				std::string message = ms.str();
				if (msg_type == MESSAGE_TYPE::CLIENT_LOGIN)
				{
					_clientInitialized = false;
					receive_CLIENT_LOGIN(ms);
					dispatch_BOUNDINGSPHERE();
					continue;
				}
				if (msg_type == MESSAGE_TYPE::CLIENT_INITIALIZED)
				{
					_clientInitialized = true;
					viewer.frame();
					continue;
				}
				else if (msg_type == MESSAGE_TYPE::VIEWMATRIX)
				{
					receive_VIEWMATRIX(ms);
					requested = true;
				}
				else if (msg_type == MESSAGE_TYPE::PROJECTIONMATRIX)
				{
					receive_PROJECTIONMATRIX(ms);
				}
				else if (msg_type == MESSAGE_TYPE::RESIZE)
				{
					receive_RESIZE(ms);
				}
			}

		}

		//_viewer->getCamera()->setViewMatrix(_cameraBuffer->getViewMatrix());
		viewer.frame();
	
		if (_clientInitialized && requested)
		{	
			//printf("sent_stamp=%d\n", _requested_stamp);
			std::stringstream ms;
			ms.write((char*)&_requested_stamp, sizeof(unsigned long));
			jpgRW->writeImage(*_cameraBuffer->_colorImage, ms);
			_socket->writeDatagram(ms.str().data(), ms.str().size(), _clientAddress, _clientPort);
			requested = false;
		}
		//osg::Vec3 eye, center, up;
		//viewer.getCamera()->getViewMatrixAsLookAt(eye, center, up);
		//osg::Vec3 look = (center - eye);
		//look.normalize();
		//printf("center=(%f,%f,%f),look=(%f,%f,%f)\n", eye.x(), eye.y(), eye.z(), look.x(), look.y(), look.z());
		//osgDB::ofstream ofs;
		//ofs.open("ljm2.jpg", std::ios::out | std::ios::binary);
		//ofs.write(ms.str().data(), ms.str().size());
		////ofs << ms.str().data();
		//ofs.close();
		////osgDB::writeImageFile(*_cameraBuffer->_colorImage, "ljm.jpg");
		//std::stringstream ms2;
		//ms2.write(ms.str().data(), ms.str().size());
		//rr = jpgRW->readImage(ms2);

		//osgDB::writeImageFile(*rr.getImage(), "ljm.jpg");
		counter++;
	}
	return 0;// viewer.run();
	
}




