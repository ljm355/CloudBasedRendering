

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
#include <osg/ShapeDrawable>
#include <math.h>
#include <QUdpSocket>
#include <QCoreApplication>
#include <QUdpSocket>
#include "ScreenOverlay.h"
#include "MessageType.h"
QUdpSocket *_socket;
osgViewer::Viewer* _viewer;
osg::ref_ptr<osg::Group> _root;
unsigned short _clientPort = 1234;
unsigned short _serverPort = 1235;
osg::ref_ptr<ScreenOverlay> _screenOverlay;
QHostAddress _serverAddress = QHostAddress::LocalHost;
QHostAddress _clientAddress = QHostAddress::LocalHost;
unsigned long _received_stamp = 0;
osg::Program* create3DModelShadersForMainScreen()
{

	osg::Program* program = new osg::Program;
	char vertexSource[] =
		"void main(void)\n"
		"{\n"
		"gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
		"}\n";
	char fragmentSource[] =
		"void main(void) \n"
		"{\n"
		"    gl_FragColor = vec4(0,0,0,0);\n"
		"}\n";

	//geode->setCullCallback(new GeomCB);
	program->setName("sphere");
	program->addShader(new osg::Shader(osg::Shader::VERTEX, vertexSource));
	program->addShader(new osg::Shader(osg::Shader::FRAGMENT, fragmentSource));
	return program;
}
void updateDummyGeometry(std::stringstream& ms)
{
	ms.seekg(STATE_MESSAGE.size() + 1, ms.beg);
	osg::Vec3 center;
	float radius;
	ms.read((char*)&center, sizeof(float) * 3);
	ms.read((char*)&radius, sizeof(float));
	osg::BoundingSphere bs(center, radius);
	osg::ref_ptr<osg::Geode> dummyGeom = new osg::Geode;
	dummyGeom->addDrawable(new osg::ShapeDrawable(new osg::Sphere(center, radius)));
	dummyGeom->getOrCreateStateSet()->setRenderBinDetails(100, "DepthSortedBin");
	dummyGeom->getOrCreateStateSet()->setAttributeAndModes(create3DModelShadersForMainScreen(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
	dummyGeom->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
	//dummyGeom->getOrCreateStateSet()->setMode(GL_ALPHA_TEST, osg::StateAttribute:ShockedN);
	if (_root->getNumChildren() > 1)
		_root->removeChild(1, 1);
	_screenOverlay->setBound(bs);
	_root->addChild(dummyGeom.get());
	_viewer->frame();
}

void dispatch_VIEWMATRIX(unsigned long stamp)
{
	std::stringstream ms;
	osg::Matrixd matview = _viewer->getCamera()->getViewMatrix();
	ms.write(STATE_MESSAGE.data(), STATE_MESSAGE.size());
	char tp = MESSAGE_TYPE::VIEWMATRIX;
	ms.write(&tp, 1);
	ms.write((char*)matview.ptr(), sizeof(double) * 16);
	ms.write((char*)&stamp, sizeof(unsigned long));
	_socket->writeDatagram(ms.str().data(), ms.str().size(), _serverAddress, _serverPort);
}

void dispatch_RESIZE(int width, int height)
{
	std::stringstream ms;
	osg::Matrixd matview = _viewer->getCamera()->getViewMatrix();
	ms.write(STATE_MESSAGE.data(), STATE_MESSAGE.size());
	char tp = MESSAGE_TYPE::RESIZE;
	ms.write(&tp, 1);
	ms.write((char*)&width, sizeof(int));
	ms.write((char*)&height, sizeof(int));
	_socket->writeDatagram(ms.str().data(), ms.str().size(), _serverAddress, _serverPort);
}

void dispatch_PROJECTIONMATRIX()
{
	std::stringstream ms;
	osg::Matrixd matproj = _viewer->getCamera()->getProjectionMatrix();
	ms.write(STATE_MESSAGE.data(), STATE_MESSAGE.size());
	char tp = MESSAGE_TYPE::PROJECTIONMATRIX;
	ms.write(&tp, 1);
	ms.write((char*)matproj.ptr(), sizeof(double) * 16);
	_socket->writeDatagram(ms.str().data(), ms.str().size(), _serverAddress, _serverPort);
}

void dispatch_CLIENT_INITIALIZED()
{
	std::stringstream ms;
	osg::Matrixd matproj = _viewer->getCamera()->getProjectionMatrix();
	ms.write(STATE_MESSAGE.data(), STATE_MESSAGE.size());
	char tp = MESSAGE_TYPE::CLIENT_INITIALIZED;
	ms.write(&tp, 1);
	_socket->writeDatagram(ms.str().data(), ms.str().size(), _serverAddress, _serverPort);
}

void dispatch_REQUESTED_FRAME_STAMP(unsigned long stamp)
{
	std::stringstream ms;
	osg::Matrixd matproj = _viewer->getCamera()->getProjectionMatrix();
	ms.write(STATE_MESSAGE.data(), STATE_MESSAGE.size());
	char tp = MESSAGE_TYPE::REQUESTED_FRAME_STAMP;
	ms.write(&tp, 1);
	ms.write((char*)&stamp, sizeof(unsigned long));
	_socket->writeDatagram(ms.str().data(), ms.str().size(), _serverAddress, _serverPort);
}
void dispatch_RECEIVED_FRAME_STAMP(unsigned long stamp)
{
	std::stringstream ms;
	osg::Matrixd matproj = _viewer->getCamera()->getProjectionMatrix();
	ms.write(STATE_MESSAGE.data(), STATE_MESSAGE.size());
	char tp = MESSAGE_TYPE::RECEIVED_FRAME_STAMP;
	ms.write(&tp, 1);
	ms.write((char*)&stamp, sizeof(unsigned long));
	_socket->writeDatagram(ms.str().data(), ms.str().size(), _serverAddress, _serverPort);
}

void dispatch_CLIENT_LOGIN()
{
	std::stringstream ms;
	ms.write(STATE_MESSAGE.data(), STATE_MESSAGE.size());
	char tp = MESSAGE_TYPE::CLIENT_LOGIN;
	ms.write(&tp, 1);
	unsigned int clientaddress = _clientAddress.toIPv4Address();
	ms.write((char*)&clientaddress,sizeof(unsigned int));
	ms.write((char*)&_clientPort, sizeof(unsigned short));
	_socket->writeDatagram(ms.str().data(), ms.str().size(), _serverAddress, _serverPort);
}
#include <QNetworkInterface>
int main(int argc, char **argv)
{
	////unsigned int clientaddress = _clientAddress.toIPv4Address();
	////clientaddress = _clientAddress.toIPv4Address();
	////printf("%d\n", clientaddress);
	////printf("%s\n", QHostAddress(clientaddress).toString().toLocal8Bit().data());
	//"127.0.0.1"
	//QNetworkInterface *inter = new QNetworkInterface();
	//QList<QHostAddress> list;
	//list = inter->allAddresses();
	//QString str;

	//for (int i = 0; i < list.size(); ++i) {
	//	str = list.at(i).toString();
	//	printf("%s\n", str.toLocal8Bit().data());
	//}
	if (argc > 1)
	{
		_serverAddress = QHostAddress(QString(argv[1]));
		_serverPort = atoi(argv[2]);
		_clientAddress = QHostAddress(QString(argv[3]));
		_clientPort = atoi(argv[4]);
	}

	QCoreApplication qapp(argc, argv);
	_socket = new QUdpSocket;
	_socket->bind(_clientAddress, _clientPort);         //ex. Address localhost, port 1234
	printf("client launched at %s:%d\n", _clientAddress.toString().toLocal8Bit().data(), _clientPort);
	printf("connecting to server at %s:%d\n", _serverAddress.toString().toLocal8Bit().data(), _serverPort);
	osgViewer::Viewer viewer;
	viewer.setUpViewAcrossAllScreens();
	viewer.setUpViewInWindow(50, 50, 1024, 768);
	_viewer = &viewer;
	_root = new osg::Group;
	_screenOverlay = new ScreenOverlay(&viewer);
	_screenOverlay->getOrCreateStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
	_screenOverlay->getOrCreateStateSet()->setRenderBinDetails(1, "DepthSortedBin");

	_root->addChild(_screenOverlay.get());
	viewer.setSceneData(_root.get());
	viewer.setThreadingModel(osgViewer::ViewerBase::ThreadingModel::ThreadPerContext);

	// add the state manipulator
	viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getOrCreateStateSet()));
	viewer.setCameraManipulator(new osgGA::TrackballManipulator);
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
	_received_stamp = counter;
	viewer.realize();
	//osgDB::Registry::instance()->loadLibrary("png"); // or "osgdb_png", not 
	osgDB::Registry::instance()->loadLibrary("jpg"); // or "osgdb_png", not 
	osg::ref_ptr<osgDB::ReaderWriter> jpgRW = osgDB::Registry::instance()->getReaderWriterForExtension("jpg");
	osgDB::ReaderWriter::ReadResult rr;
	bool requestReceived = true;
	int skippedFrames = 0;
	dispatch_CLIENT_LOGIN();
	while (!viewer.done())
	{

		while (_socket->hasPendingDatagrams())
		{
			QByteArray Buffer;
			Buffer.resize(_socket->pendingDatagramSize());
			QHostAddress sender;
			quint16 senderPort;
			_socket->readDatagram(Buffer.data(), Buffer.size(), &_serverAddress, &_serverPort);
			std::stringstream ms;
			ms.write(Buffer.data(), Buffer.size());
			if (ms.str().substr(0, STATE_MESSAGE.size()) == STATE_MESSAGE)
			{
				char msg_type = ms.str().data()[STATE_MESSAGE.size()];
				std::string message = ms.str();
				if (msg_type == MESSAGE_TYPE::BOUNDINGSPHERE)
				{
					updateDummyGeometry(ms);
					dispatch_CLIENT_INITIALIZED();
					dispatch_PROJECTIONMATRIX();
					//dispatch_VIEWMATRIX(counter);
					viewer.home();
				}
			}
			else
			{
				//printf("_received_stamp=%d\n", _received_stamp);
				ms.seekg(0, ms.beg);
				ms.read((char*)(&_received_stamp), sizeof(unsigned long));
				//ms.seekg(sizeof(unsigned long), ms.beg);
				//ms.seekg(0, ms.beg);
				rr = jpgRW->readImage(ms);
				osg::ref_ptr<osg::Image> img = rr.takeImage();
				_screenOverlay->_frameTexture->setImage(img.get());
				requestReceived = true;
			
				unsigned int width = _viewer->getCamera()->getViewport()->width();
				unsigned int height = _viewer->getCamera()->getViewport()->height();
				if (img->s() != width || img->t() != height)
				{
					//printf("width=%d,height=%d\n", width, height);
					dispatch_RESIZE(width, height);
				}
				dispatch_PROJECTIONMATRIX();
			}
		}
		if (_root->getNumChildren() < 2)
			continue;
		if (requestReceived || skippedFrames > 2)
		{
			dispatch_VIEWMATRIX(counter);
			counter++;
			requestReceived = false;
			skippedFrames = 0;
		}
		skippedFrames++;
		viewer.frame();
		osg::Vec3 eye, center, up;
		viewer.getCamera()->getViewMatrixAsLookAt(eye, center, up);
		osg::Vec3 look = (center - eye);
		look.normalize();
		//printf("center=(%f,%f,%f),look=(%f,%f,%f)\n", eye.x(), eye.y(), eye.z(), look.x(), look.y(), look.z());
	}
	return 0;// viewer.run();
	
}




