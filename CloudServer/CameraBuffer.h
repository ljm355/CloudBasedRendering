#pragma once
#include <Windows.h>
#include <osg/Camera>
#include <osg/Texture2D>
#include <osg/GraphicsContext>
#include <osg/Vec3d>
class CameraBuffer : public osg::Camera
{
public:
	osg::ref_ptr<osg::Texture2D> _colorTexture;
	osg::ref_ptr<osg::Image> _colorImage;
	std::string _name;
	osg::Vec3d _pos;//position of camera
	osg::Vec3d _dir;//viewing direction of camera
	osg::Vec3d _up;//the _up vector for use in building a view matrix
	CameraBuffer();
	void setupBuffer(int w, int h);
	void update();
	osg::Matrixd getCombinedMatrix();
	//w: width of camera buffer
	//h: height of camera buffer
	static CameraBuffer* create(int w, int h);
	static CameraBuffer* createSlave(int w, int h, osg::GraphicsContext* context = NULL);
};