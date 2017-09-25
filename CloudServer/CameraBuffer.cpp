#include "CameraBuffer.h"

#include <osg/ComputeBoundsVisitor>

CameraBuffer::CameraBuffer()
	:osg::Camera()
{

}

void CameraBuffer::setupBuffer(int w, int h)
{
	if (_colorImage.valid() && _colorImage->s() == w && _colorImage->t() == h)
		return;
	_colorTexture = new osg::Texture2D;
	_colorTexture->setTextureSize(w, h);
	_colorTexture->setResizeNonPowerOfTwoHint(false);
	_colorTexture->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
	_colorTexture->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
	_colorTexture->setWrap(osg::Texture2D::WRAP_S, osg::Texture2D::REPEAT);
	_colorTexture->setWrap(osg::Texture2D::WRAP_T, osg::Texture2D::REPEAT);
	_colorTexture->setDataVariance(osg::Object::DYNAMIC);
	_colorTexture->setInternalFormat(GL_RGB);
	_colorTexture->setSourceFormat(GL_RGB);
	_colorTexture->setSourceType(GL_UNSIGNED_BYTE);
	_colorImage = new osg::Image;
	_colorImage->allocateImage(w, h, 1, GL_RGB, GL_UNSIGNED_BYTE);
	_colorTexture->setImage(_colorImage.get());

	setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	setClearColor(osg::Vec4(0, 0, 0, 0));
	//camera->setClearDepth(0);
	//camera->setClearColor(osg::Vec4(0.53f, 0.85f, 1.0f, 0.9f));				// Background
	setReferenceFrame(osg::Transform::ABSOLUTE_RF_INHERIT_VIEWPOINT);
	setViewport(0, 0, w, h);
	setRenderOrder(osg::Camera::PRE_RENDER);
	setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);

	attach(osg::Camera::COLOR_BUFFER0, _colorTexture.get());
	attach(osg::Camera::COLOR_BUFFER0, _colorImage.get());

}

void CameraBuffer::update()
{
	//osg::Matrix localOffset;
	//_dir.normalize();
	//osg::Vec3 crossdir = _dir ^ osg::Vec3(0, 0, -1);
	//_up = _dir ^ crossdir;
	////printf("%f,%f,%f\n", up.x(), up.y(), up.z());

	//localOffset.makeLookAt(_pos, _pos + _dir * 100, _up);
	//osg::Matrix viewMatrix = localOffset;
	//setReferenceFrame(osg::Camera::ABSOLUTE_RF);
	//float nearDist = 0.1;
	////setProjectionMatrixAsFrustum(-nearDist, nearDist, -nearDist, nearDist, nearDist, 10000.0);
	//setProjectionMatrixAsPerspective(45.0, 1.0, 0.5, 10000);
	//setViewMatrix(viewMatrix);
	////setClearColor(osg::Vec4(0, 0, 0, 0));
}

osg::Matrixd CameraBuffer::getCombinedMatrix()
{
	
	return getViewMatrix() * getProjectionMatrix();
}

CameraBuffer * CameraBuffer::create(int w, int h)
{
	CameraBuffer* camera = new CameraBuffer;
	camera->setupBuffer(w, h);
	return camera;
}

CameraBuffer * CameraBuffer::createSlave(int w, int h, osg::GraphicsContext * context)
{
	CameraBuffer* camera = new CameraBuffer;
	if (!context)
	{
		osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
		traits->x = 0;
		traits->y = 0;
		traits->width = w;
		traits->height = h;
		traits->windowDecoration = true; // window border etc.
		traits->doubleBuffer = true;
		traits->sharedContext = 0;
		traits->vsync = false;
		context = osg::GraphicsContext::createGraphicsContext(traits.get());
	}

	camera->setViewport(0, 0, w, h);
	camera->setGraphicsContext(context);
	camera->setupBuffer(w, h);

	return camera;
}
