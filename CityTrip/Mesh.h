#include<iostream>
#include<map>
#include<sstream>
#include<assimp\vector3.h>
#include<assimp\Importer.hpp>
#include<assimp\scene.h>
#include<assimp\postprocess.h>
#include<assimp\cimport.h>
#include <IL\il.h>
#include"freeglut\freeglut.h"
#include "atlbase.h" 
#include "atlstr.h"


#define aisgl_min(x,y) (x<y?x:y)
#define aisgl_max(x,y) (y>x?y:x)

using namespace std;

class Mesh {
public:
	Mesh(string path) {
		modelpath = path;
	}

	~Mesh() {}

	int loadasset() {
		const char*path = modelpath.c_str();
		scene = aiImportFile(path, aiProcessPreset_TargetRealtime_MaxQuality);

		if (scene) {
			get_bounding_box(&scene_min, &scene_max);
			scene_center.x = (scene_min.x + scene_max.x) / 2.0f;
			scene_center.y = (scene_min.y + scene_max.y) / 2.0f;
			scene_center.z = (scene_min.z + scene_max.z) / 2.0f;
			return 0;
		}
		return 1;
	}

	int Render() {
		//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	// Clear The Screen And The Depth Buffer
		glLoadIdentity();	

		glTranslatef(0.0f, 0.0f, -20.0f);

		
		glPushMatrix();
		glRotatef(180, 1.0f, 0.0f, 0.0f);
		
		/*glRotatef(yrot, 0.0f, 1.0f, 0.0f);
		glRotatef(zrot, 0.0f, 0.0f, 1.0f);*/

		//xrot+=0.3f;
		//yrot += 0.2f;
		//zrot+=0.4f;

		recursive_render(scene, scene->mRootNode, 0.05f);


		glPopMatrix();
		return TRUE;
	}

	int LoadGLTextures()
	{
		ILboolean success = true;

		/* Before calling ilInit() version should be checked. */
		if (ilGetInteger(IL_VERSION_NUM) < IL_VERSION)
		{
			/// wrong DevIL version ///
			return -1;
			cout << "version";
		}

		ilInit(); /* Initialization of DevIL */


				  /* getTexture Filenames and Numb of Textures */
		for (unsigned int m = 0; m<scene->mNumMaterials; m++)
		{
			int texIndex = 0;
			aiReturn texFound = AI_SUCCESS;

			aiString path;	// filename

			while (texFound == AI_SUCCESS)
			{
				texFound = scene->mMaterials[m]->GetTexture(aiTextureType_DIFFUSE, texIndex, &path);
				textureIdMap[path.data] = NULL; //fill map with textures, pointers still NULL yet
				texIndex++;
			}
		}

		int numTextures = textureIdMap.size();

		/* array with DevIL image IDs */
		ILuint* imageIds = NULL;
		imageIds = new ILuint[numTextures];

		/* generate DevIL Image IDs */
		ilGenImages(numTextures, imageIds); /* Generation of numTextures image names */

											/* create and fill array with GL texture ids */
		textureIds = new GLuint[numTextures];
		glGenTextures(numTextures, textureIds); /* Texture name generation */

												/* get iterator */
		std::map<std::string, GLuint*>::iterator itr = textureIdMap.begin();

		std::string basepath = getBasePath(modelpath);
		for (int i = 0; i<numTextures; i++)
		{

			//save IL image ID
			std::string filename = (*itr).first;  // get filename
			(*itr).second = &textureIds[i];	  // save texture id for filename in map
			itr++;								  // next texture


			ilBindImage(imageIds[i]); /* Binding of DevIL image name */
			std::string fileloc = basepath + filename;	/* Loading of image */
			/*TCHAR aaa[100];
			MultiByteToWideChar(0, 0, fileloc.c_str(), 100, aaa, 200);
			success = ilLoadImage(aaa);*/
			const char* filelocation = fileloc.c_str();
			size_t len = strlen(filelocation) + 1;
			size_t converted = 0;
			// 准备转换的对象
			ilBindImage(imageIds[i]); /* Binding of DevIL image name */
			success = ilLoadImage(fileloc.c_str());

			if (success) /* If no error occurred: */
			{
				// Convert every colour component into unsigned byte.If your image contains 
				// alpha channel you can replace IL_RGB with IL_RGBA
				success = ilConvertImage(IL_RGB, IL_UNSIGNED_BYTE);
				if (!success)
				{
					/* Error occurred */
					cout << "can't";
					return -1;
				}
				// Binding of texture name
				glBindTexture(GL_TEXTURE_2D, textureIds[i]);
				// redefine standard texture values
				// We will use linear interpolation for magnification filter
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				// We will use linear interpolation for minifying filter
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				// Texture specification
				glTexImage2D(GL_TEXTURE_2D, 0, ilGetInteger(IL_IMAGE_BPP), ilGetInteger(IL_IMAGE_WIDTH),
					ilGetInteger(IL_IMAGE_HEIGHT), 0, ilGetInteger(IL_IMAGE_FORMAT), GL_UNSIGNED_BYTE,
					ilGetData());//gamma矫正
				// we also want to be able to deal with odd texture dimensions
				glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
				glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
				glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
				glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
			}
			else
			{
				/* Error occurred */

			}
		}
		// Because we have already copied image data into texture data  we can release memory used by image.
		ilDeleteImages(numTextures, imageIds);

		// Cleanup
		delete[] imageIds;
		imageIds = NULL;

		return TRUE;
	}

	float getAngle() {
		return angle;
	}
	float getScare() {
		float tmp;

		tmp = scene_max.x - scene_min.x;
		tmp = aisgl_max(scene_max.y - scene_min.y, tmp);
		tmp = aisgl_max(scene_max.z - scene_min.z, tmp);
		tmp = 1.f / tmp;

		return tmp;
	}
	float getCenterX() {
		return scene_center.x;
	}
	float getCenterY() {
		return scene_center.y;
	}
	float getCenterZ() {
		return scene_center.z;
	}

private:
	const aiScene* scene;
	GLuint scene_list = 0;
	aiVector3D scene_min, scene_max, scene_center;
	string modelpath;
	GLuint* textureIds;
	std::map<std::string, GLuint*> textureIdMap;

	float angle;

	void get_bounding_box_for_node(const aiNode* nd, aiVector3D* min, aiVector3D* max, aiMatrix4x4* trafo) {
		aiMatrix4x4 prev;
		unsigned int n = 0, t;

		prev = *trafo;
		aiMultiplyMatrix4(trafo, &nd->mTransformation);

		for (; n < nd->mNumMeshes; ++n) {
			const struct aiMesh* mesh = scene->mMeshes[nd->mMeshes[n]];
			for (t = 0; t < mesh->mNumVertices; ++t) {

				aiVector3D tmp = mesh->mVertices[t];
				aiTransformVecByMatrix4(&tmp, trafo);

				min->x = aisgl_min(min->x, tmp.x);
				min->y = aisgl_min(min->y, tmp.y);
				min->z = aisgl_min(min->z, tmp.z);

				max->x = aisgl_max(max->x, tmp.x);
				max->y = aisgl_max(max->y, tmp.y);
				max->z = aisgl_max(max->z, tmp.z);
			}
		}

		for (n = 0; n < nd->mNumChildren; ++n) {
			get_bounding_box_for_node(nd->mChildren[n], min, max, trafo);
		}
		*trafo = prev;
	}

	void get_bounding_box(aiVector3D* min, aiVector3D* max)
	{
		aiMatrix4x4 trafo;
		aiIdentityMatrix4(&trafo);

		min->x = min->y = min->z = 1e10f;
		max->x = max->y = max->z = -1e10f;
		get_bounding_box_for_node(scene->mRootNode, min, max, &trafo);
	}

	void Color4f(const aiColor4D *color) {
		glColor4f(color->r, color->g, color->b, color->a);
	}

	void color4_to_float4(const aiColor4D *c, float f[4])
	{
		f[0] = c->r;
		f[1] = c->g;
		f[2] = c->b;
		f[3] = c->a;
	}

	void set_float4(float f[4], float a, float b, float c, float d)
	{
		f[0] = a;
		f[1] = b;
		f[2] = c;
		f[3] = d;
	}

	void recursive_render(const aiScene *sc, const aiNode* nd, float scale) {
		unsigned int i;
		unsigned int n = 0, t;
		aiMatrix4x4 m = nd->mTransformation;

		aiMatrix4x4 m2;
		aiMatrix4x4::Scaling(aiVector3D(scale, scale, scale), m2);
		m = m * m2;

		// update transform
		m.Transpose();
		glPushMatrix();
		glMultMatrixf((float*)&m);

		// draw all meshes assigned to this node
		for (; n < nd->mNumMeshes; ++n)
		{
			const struct aiMesh* mesh = scene->mMeshes[nd->mMeshes[n]];

			apply_material(sc->mMaterials[mesh->mMaterialIndex]);


			if (mesh->mNormals == NULL)
			{
				glDisable(GL_LIGHTING);
			}
			else
			{
				glEnable(GL_LIGHTING);
			}

			if (mesh->mColors[0] != NULL)
			{
				glEnable(GL_COLOR_MATERIAL);
			}
			else
			{
				glDisable(GL_COLOR_MATERIAL);
			}

			for (t = 0; t < mesh->mNumFaces; ++t) {
				const struct aiFace* face = &mesh->mFaces[t];
				GLenum face_mode;

				switch (face->mNumIndices)
				{
				case 1: face_mode = GL_POINTS; break;
				case 2: face_mode = GL_LINES; break;
				case 3: face_mode = GL_TRIANGLES; break;
				default: face_mode = GL_POLYGON; break;
				}

				glBegin(face_mode);

				for (i = 0; i < face->mNumIndices; i++)		
				{
					int vertexIndex = face->mIndices[i];	
					if (mesh->mColors[0] != NULL)
						Color4f(&mesh->mColors[0][vertexIndex]);
					if (mesh->mNormals != NULL)

						if (mesh->HasTextureCoords(0))		
						{
							glTexCoord2f(mesh->mTextureCoords[0][vertexIndex].x, 1 - mesh->mTextureCoords[0][vertexIndex].y);
						}

					glNormal3fv(&mesh->mNormals[vertexIndex].x);
					glVertex3fv(&mesh->mVertices[vertexIndex].x);
				}
				glEnd();
			}
		}

		// draw all children
		for (n = 0; n < nd->mNumChildren; ++n)
		{
			recursive_render(sc, nd->mChildren[n], scale);
		}

		glPopMatrix();
	}

	void apply_material(const struct aiMaterial *mtl) {
		float c[4];

		GLenum fill_mode;
		int ret1, ret2;
		aiColor4D diffuse;
		aiColor4D specular;
		aiColor4D ambient;
		aiColor4D emission;
		float shininess, strength;
		int two_sided;
		int wireframe;
		unsigned int max;	// changed: to unsigned

		int texIndex = 0;
		aiString texPath;	//contains filename of texture

		if (AI_SUCCESS == mtl->GetTexture(aiTextureType_DIFFUSE, texIndex, &texPath))
		{
			//bind texture
			unsigned int texId = *textureIdMap[texPath.data];
			glBindTexture(GL_TEXTURE_2D, texId);
		}

		/*set_float4(c, 0.8f, 0.8f, 0.8f, 1.0f);
		if (AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_DIFFUSE, &diffuse))
			color4_to_float4(&diffuse, c);
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, c);

		set_float4(c, 0.0f, 0.0f, 0.0f, 1.0f);
		if (AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_SPECULAR, &specular))
			color4_to_float4(&specular, c);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, c);

		set_float4(c, 0.2f, 0.2f, 0.2f, 1.0f);
		if (AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_AMBIENT, &ambient))
			color4_to_float4(&ambient, c);
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, c);

		set_float4(c, 0.0f, 0.0f, 0.0f, 1.0f);
		if (AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_EMISSIVE, &emission))
			color4_to_float4(&emission, c);
		glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, c);*/

	/*	max = 1;
		ret1 = aiGetMaterialFloatArray(mtl, AI_MATKEY_SHININESS, &shininess, &max);
		max = 1;
		ret2 = aiGetMaterialFloatArray(mtl, AI_MATKEY_SHININESS_STRENGTH, &strength, &max);
		if ((ret1 == AI_SUCCESS) && (ret2 == AI_SUCCESS))
			glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess * strength);
		else {
			glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0.0f);
			set_float4(c, 0.0f, 0.0f, 0.0f, 0.0f);
			glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, c);
		}
*/
		max = 1;
		if (AI_SUCCESS == aiGetMaterialIntegerArray(mtl, AI_MATKEY_ENABLE_WIREFRAME, &wireframe, &max))
			fill_mode = wireframe ? GL_LINE : GL_FILL;
		else
			fill_mode = GL_FILL;
		glPolygonMode(GL_FRONT_AND_BACK, fill_mode);

		max = 1;
		if ((AI_SUCCESS == aiGetMaterialIntegerArray(mtl, AI_MATKEY_TWOSIDED, &two_sided, &max)) && two_sided)
			glEnable(GL_CULL_FACE);
		else
			glDisable(GL_CULL_FACE);
	}


	string getBasePath(const std::string& path) {
		size_t pos = path.find_last_of("\\/");
		return (std::string::npos == pos) ? "" : path.substr(0, pos + 1);
	}
};
