#pragma once
#include <Windows.h>
#include <osg/Texture2D>
#include <osg/Geometry>
#include <osg/Camera>
#include <osg/Geode>
#include <osgViewer/Viewer>
class ScreenOverlay : public osg::Geode
{
public:
	ScreenOverlay(osgViewer::Viewer* viewer);
	~ScreenOverlay();
	virtual osg::BoundingSphere computeBound() const;
	osg::ref_ptr<osg::Texture2D> _frameTexture;
	void setBound(osg::BoundingSphere bs);

};

