#include "ScreenOverlay.h"
#include <osgDB/ReadFile>
#include <osgGA/GUIEventHandler>

ScreenOverlay::ScreenOverlay(osgViewer::Viewer* viewer)
{

	osg::ref_ptr<osg::Geometry> geom = new osg::Geometry();
	osg::Vec3Array* vertices = new osg::Vec3Array();
	osg::Vec3Array* normals = new osg::Vec3Array();
	vertices->push_back(osg::Vec3(-1, -1, 1));
	vertices->push_back(osg::Vec3(-1, 1, 1));
	vertices->push_back(osg::Vec3(1, -1, 1));
	vertices->push_back(osg::Vec3(1, -1, 1));
	vertices->push_back(osg::Vec3(-1, 1, 1));
	vertices->push_back(osg::Vec3(1, 1, 1));
	normals->push_back(osg::Vec3(0, 0, 1));
	geom->setVertexArray(vertices);
	geom->setNormalArray(normals);
	geom->setNormalBinding(osg::Geometry::BIND_OVERALL);
	geom->setCullingActive(false);
	this->setCullingActive(false);
	this->getOrCreateStateSet()->setMode(GL_CULL_FACE,
		osg::StateAttribute::OFF | osg::StateAttribute::OVERRIDE);
	geom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, 0, 6));
	this->addDrawable(geom.get());
	this->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
	//this->getOrCreateStateSet()->setTextureAttributeAndModes(0, tex.get(), osg::StateAttribute::ON);

	osg::Program* program = new osg::Program;

	char vertexSource[] =
		"void main(void)\n"
		"{\n"
		"   gl_TexCoord[0] = vec4(gl_Vertex.x,gl_Vertex.y,0,1.0);\n"
		"   gl_Position   = vec4(gl_Vertex.x,gl_Vertex.y,0,1);\n"
		"}\n";
	char fragmentSource[] =
		"uniform sampler2D texture0;\n"
		"uniform sampler2D texture1;\n"
		"void main(void) \n"
		"{\n"
		"    vec2 uv = gl_TexCoord[0].xy; \n"
		"    uv = uv * 0.5 + 0.5;\n"
		"    vec4 color =   texture2D(texture0, uv);\n"
		"    gl_FragColor = vec4(color.rgb,1);\n"
		"}\n";
	program->setName("overlay");
	program->addShader(new osg::Shader(osg::Shader::VERTEX, vertexSource));
	program->addShader(new osg::Shader(osg::Shader::FRAGMENT, fragmentSource));
	this->getOrCreateStateSet()->setAttributeAndModes(program, osg::StateAttribute::ON);
	this->getOrCreateStateSet()->addUniform(new osg::Uniform("texture0", 0));
	this->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::OFF);
	_frameTexture = new osg::Texture2D;
	_frameTexture->setWrap(osg::Texture2D::WRAP_S, osg::Texture2D::CLAMP_TO_BORDER);
	_frameTexture->setWrap(osg::Texture2D::WRAP_T, osg::Texture2D::CLAMP_TO_BORDER);
	_frameTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
	_frameTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);
	_frameTexture->setResizeNonPowerOfTwoHint(false);
	_frameTexture->setDataVariance(osg::Object::DYNAMIC);
	getOrCreateStateSet()->setTextureAttributeAndModes(0, _frameTexture.get(), osg::StateAttribute::ON);
}


ScreenOverlay::~ScreenOverlay()
{

}

osg::BoundingSphere ScreenOverlay::computeBound() const
{
	return _boundingSphere;
}

void ScreenOverlay::setBound(osg::BoundingSphere bs)
{
	_boundingSphereComputed = true;
	_boundingSphere = bs;
}

