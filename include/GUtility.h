//----------------------------------------------------------------------------------//
// OgreRecast Demo - A demonstration of integrating Recast Navigation Meshes		//
//					 with the Ogre3D rendering engine.								//
//																					//
//	This file was either Created by or Modified by :								//
//													Paul A Wilson 					//
//			All contents are Copyright (C) 2010 Paul A Wilson						//
//			Except where otherwise mentioned or where previous						//
//			copyright exists. In the case of pre-existing copyrights				//
//			all rights remain with the original Authors, and this is				//
//			to be considered a derivative work.										//
//																					//
//	Contact Email	:	paulwilson77@dodo.com.au									//
//																					//
// This 'SOFTWARE' is provided 'AS-IS', without any express or implied				//
// warranty.  In no event will the authors be held liable for any damages			//
// arising from the use of this software.											//
// Permission is granted to anyone to use this software for any purpose,			//
// including commercial applications, and to alter it and redistribute it			//
// freely, subject to the following restrictions:									//
// 1. The origin of this software must not be misrepresented; you must not			//
//    claim that you wrote the original software. If you use this software			//
//    in a product, an acknowledgment in the product documentation would be			//
//    appreciated but is not required.												//
// 2. Altered source versions must be plainly marked as such, and must not be		//
//    misrepresented as being the original software.								//
// 3. This notice may not be removed or altered from any source distribution.		//
//																					//
//----------------------------------------------------------------------------------//


 /*
 *		Original Code obtained from Ogitor source. - only minor modifications made 
 *               http://www.ogitor.org
 *
 */
/*/////////////////////////////////////////////////////////////////////////////////
/// An
///    ___   ____ ___ _____ ___  ____
///   / _ \ / ___|_ _|_   _/ _ \|  _ \
///  | | | | |  _ | |  | || | | | |_) |
///  | |_| | |_| || |  | || |_| |  _ <
///   \___/ \____|___| |_| \___/|_| \_\
///                              File
///
/// Copyright (c) 2008-2010 Ismail TARIM <ismail@royalspor.com> and the Ogitor Team
//
/// The MIT License
///
/// Permission is hereby granted, free of charge, to any person obtaining a copy
/// of this software and associated documentation files (the "Software"), to deal
/// in the Software without restriction, including without limitation the rights
/// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
/// copies of the Software, and to permit persons to whom the Software is
/// furnished to do so, subject to the following conditions:
///
/// The above copyright notice and this permission notice shall be included in
/// all copies or substantial portions of the Software.
///
/// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
/// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
/// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
/// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
/// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
/// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
/// THE SOFTWARE.
////////////////////////////////////////////////////////////////////////////////*/

#pragma once

#include "Ogre.h"
#include "tinyxml.h"
#include "SharedData.h"

class TemplateUtils
{
    public:
        /**
        * Sets full path to the Ogitor' executable
        * @param path full path to the Ogitor' executable
        */
        static void SetExePath(const Ogre::String &path) { mExePath = path; } 
        /**
        * Fetches full path to the Ogitor' executable
        * @return full path to the Ogitor' executable
        */
        static Ogre::String GetExePath() { return mExePath; }
        /**
        * Fetches full path to specified directory
        * @param dirname directory name
        * @return full path to specified directory
        * @todo Recommend renaming to GetFullPath
        */
        static Ogre::String QualifyPath(const Ogre::String &dirname);
        /**
        * Fetches relative path from one directory to another
        * @param pathFrom originating path
        * @param pathTo destination path
        * @return relative path from origin to destination directories
        */
        static Ogre::String GetRelativePath(const Ogre::String pathFrom,const Ogre::String pathTo);
        /**
        * Parses a string of strings into the list
        * @param str string containing multiple entries 
        * @param list a list to put strings into
        */
        static void ParseStringVector(Ogre::String &str, Ogre::StringVector &list);
        /**
        * Parses a string of strings into the list (UTF based)
        * @param str string containing multiple entries 
        * @param list a list to put strings into
        */
        static void ParseUTFStringVector(Ogre::UTFString &str , UTFStringVector &list);
        /**
        * Cleans a path of un-needed characters/symbols
        * @param path to be cleaned
        */
        static void CleanPath(Ogre::String &path);
        /**
        * Extracts file name from given path
        * @param path path containing the filename
        * @return file name
        * @todo Recommend renaming to GetFileName
        */
        static Ogre::String ExtractFileName(const Ogre::String &path);
        /**
        * Extracts file path from given path
        * @param path path containing the the file path
        * @return file path
        * @todo Recommend renaming to GetFilePath
        */
        static Ogre::String ExtractFilePath(const Ogre::String &path);
        /**
        * Fetches information about OGRE-based mesh
        * @param mesh mesh to retrieve information about
        * @param vertex_count number of vertices to retrieve
        * @param vertices raw Ogre::Vector3 array pointer to mesh' vertices
        * @param index_count number of indices to retrieve
        * @param indices raw ulong array pointer to mesh' indices
        * @param position mesh position to retrieve
        * @param orient mesh orientation to retrieve
        * @param scale mesh scale to retrieve
        */
        static void GetMeshData(const Ogre::MeshPtr mesh, size_t &vertex_count, size_t &index_count,
								const Ogre::Vector3 &position = Ogre::Vector3::ZERO, const Ogre::Quaternion &orient = Ogre::Quaternion::IDENTITY,
								const Ogre::Vector3 &scale = Ogre::Vector3::UNIT_SCALE);
        /**
        * Fetches information about OGRE-based mesh
        * @param entity entity to retrieve information about
        * @param vertex_count number of vertices to retrieve
        * @param vertices raw Ogre::Vector3 array pointer to mesh' vertices
        * @param index_count number of indices to retrieve
        * @param indices raw ulong array pointer to mesh' indices
        * @param position mesh position to retrieve
        * @param orient mesh orientation to retrieve
        * @param scale mesh scale to retrieve
        */
        static void GetMeshDataEx(const Ogre::Entity *entity, size_t &vertex_count, size_t &index_count,
                                const Ogre::Vector3 &position = Ogre::Vector3::ZERO, const Ogre::Quaternion &orient = Ogre::Quaternion::IDENTITY,
								const Ogre::Vector3 &scale = Ogre::Vector3::UNIT_SCALE);

		 /**
        * Fetches information about OGRE-based mesh
        * @param mesh mesh to retrieve information about
        * @param vertex_count number of vertices to retrieve
        * @param vertices raw Ogre::Vector3 array pointer to mesh' vertices
        * @param index_count number of indices to retrieve
        * @param indices raw ulong array pointer to mesh' indices
        * @param position mesh position to retrieve
        * @param orient mesh orientation to retrieve
        * @param scale mesh scale to retrieve
        */
		static void getMeshInformation(const Ogre::MeshPtr mesh, size_t &vertex_count, Ogre::Vector3* &vertices,
									size_t &index_count, unsigned long* &indices, const Ogre::Vector3 &position = Ogre::Vector3::ZERO, 
									const Ogre::Quaternion &orient = Ogre::Quaternion::IDENTITY, const Ogre::Vector3 &scale = Ogre::Vector3::UNIT_SCALE);

		 /**
        * Fetches information about OGRE-based mesh
        * @param mesh mesh to retrieve information about
        * @param vertex_count number of vertices to retrieve
        * @param vertices raw Ogre::Vector3 array pointer to mesh' vertices
        * @param index_count number of indices to retrieve
        * @param indices raw ulong array pointer to mesh' indices
        * @param position mesh position to retrieve
        * @param orient mesh orientation to retrieve
        * @param scale mesh scale to retrieve
        */
		static void getMeshInformationEX(const Ogre::MeshPtr mesh, size_t &vertex_count, Ogre::Vector3* &vertices,
									size_t &index_count, unsigned long* &indices, const Ogre::Vector3 &position = Ogre::Vector3::ZERO, 
									const Ogre::Quaternion &orient = Ogre::Quaternion::IDENTITY, const Ogre::Vector3 &scale = Ogre::Vector3::UNIT_SCALE);

		 /**
        * Fetches information about OGRE-based mesh
        * @param entity entity to retrieve information about
        * @param vertex_count number of vertices to retrieve
        * @param vertices raw Ogre::Vector3 array pointer to mesh' vertices
        * @param index_count number of indices to retrieve
        * @param indices raw ulong array pointer to mesh' indices
        * @param position mesh position to retrieve
        * @param orient mesh orientation to retrieve
        * @param scale mesh scale to retrieve
        */
		static void getMeshInformationEXA(Ogre::Entity &entity, size_t &vertex_count, Ogre::Vector3* &vertices, 
									size_t &index_count, unsigned long* &indices, const Ogre::Vector3 &position = Ogre::Vector3::ZERO,
									const Ogre::Quaternion &orient = Ogre::Quaternion::IDENTITY, const Ogre::Vector3 &scale = Ogre::Vector3::UNIT_SCALE);


        /**
        * Attempts to pick an entity within scene using ray cast from the mouse
        * @param mRaySceneQuery a ray scene query object 
        * @see Ogre::RaySceneQuery
        * @param ray ray cast from the mouse  
        * @see COgitorsRoot::GetMouseRay
        * @param result a result of entities that were intersected with the ray
        * @param hitpoint a closest point of the ray/entity intersection to the camera
        * @param max_distance check only objects closer than this value
        * @param excludeobject the name of the object to exclude from hit test
        * @return true if any entity was intersected with the ray, otherwise false
        */
        static bool PickEntity(Ogre::RaySceneQuery* mRaySceneQuery, Ogre::Ray &ray, Ogre::Entity **result, Ogre::Vector3 &hitpoint, const Ogre::String& excludeobject = "", Ogre::Real max_distance = -1.0f);
        /**
        * Attempts to pick an entity within scene using ray cast from the mouse
        * @param mRaySceneQuery a ray scene query object 
        * @see Ogre::RaySceneQuery
        * @param ray ray cast from the mouse  
        * @see COgitorsRoot::GetMouseRay
        * @param result a result of entities that were intersected with the ray
        * @param hitpoint a closest point of the ray/entity intersection to the camera
        * @param max_distance check only objects closer than this value
        * @param excludeobjects the list of names of the objects to exclude from hit test
        * @return true if any entity was intersected with the ray, otherwise false
        */
        static bool PickEntity(Ogre::RaySceneQuery* mRaySceneQuery, Ogre::Ray &ray, Ogre::Entity **result, Ogre::Vector3 &hitpoint, const Ogre::StringVector& excludeobjects, Ogre::Real max_distance = -1.0f);
        /**
        * Attempts to find the index of the submesh containing the point hitpoint
        * @param pEntity the parent Entity Object  
        * @see Ogre::Entity
         * @param hitpoint the location on the Entity
        * @return the index of the submesh containing the hitpoint
        */
        static int PickSubMesh(Ogre::Ray& ray, Ogre::Entity* pEntity);
        /**
        * Test if specified ray has intersected with anything on the scene
        * @param mRaySceneQuery ray scene query object helper 
        * @param ray a ray that is to be tested
        * @param hitposition location of an intersect (if any)
        * @return true if ray had intersected with anything, otherwise false
        */
        static bool WorldIntersect(Ogre::RaySceneQuery* mRaySceneQuery, Ogre::Ray &ray, Ogre::Vector3 &hitposition);
        /**
        * Fetches internal vertex and index buffers
        * @param VertexBuffer address of pointer to vertex buffers to be set
        * @param IndexBuffer address of pointer to index buffers to be set
        */
        static void GetBuffers(Ogre::Vector3 **VertexBuffer, unsigned long **IndexBuffer)
        {
            *VertexBuffer = mVertexBuffer;
            *IndexBuffer = mIndexBuffer;
        }
        /**
        * Frees internal vertex and index buffers
        */
        static void FreeBuffers()
        {
            OGRE_FREE(mVertexBuffer, Ogre::MEMCATEGORY_GEOMETRY);
            mVertexBuffer = 0;
            mVertexBufferSize = 0;
            OGRE_FREE(mIndexBuffer, Ogre::MEMCATEGORY_GEOMETRY);
            mIndexBuffer = 0;
            mIndexBufferSize = 0;
        }
        /**
        * Returns the value of a property as string
        * @param value the PropertyValue to be converted 
        * @return the string value
        */
        static Ogre::String GetValueString( /* OgitorsPropertyValue& value */ );
        /**
        * Returns a string containing xml structure ofa custom property set
        * @param set the set that will be used to create xml structure
        * @param indentation space to be left at the beginning of each line
        */
        static Ogre::String GetCustomPropertySaveString( /* OgitorsCustomPropertySet *set ,*/ int indentation);
        /**
        * Returns a string containing xml structure ofa custom property set
        * @param set the set that will be used to create xml structure
        * @param indentation space to be left at the beginning of each line
        */
        static void ReadCustomPropertySet(TiXmlElement *element /*, OgitorsCustomPropertySet *set*/ );
        /**
        * Returns a string containing xml structure of an object
        * @param object the object that will be used to create xml structure
        * @param useobjid will there be a object_id parameter?
        * @param addparent will there be a parentnode parameter?
        * @return returns a string containing xml syntax created from the object
        */
        static Ogre::String GetObjectSaveStringV2(/* CBaseEditor *object  ,*/ int indentation, bool useobjid, bool addparent);
        /**
        * Returns a string containing xml structure ofa custom property set for DotScene Format
        * @param set the set that will be used to create xml structure
        * @param indentation space to be left at the beginning of each line
        */
        static Ogre::String GetUserDataSaveString(/* OgitorsCustomPropertySet *set ,*/ int indentation);

		//-----------------------------------------------------------------------
		// CHEAP VECTOR MANIPULATION METHODS

		/**
		*	Zero a vector
		*/
		static void vzero(float *v);

		/**
		*	Set a vector's scalar values
		*/
		static void vset(float *v, float x, float y, float z);

		/**
		*	Subtract one vector from another
		*/
		static void vsub(const float *src1, const float *src2, float *dst);

		/**
		*	Copy a vector
		*/
		static void vcopy(float *v1, const float *v2);

		/**
		*	Cross-multiply two vectors
		*/
		static void vcross(const float *v1, const float *v2, float *cross);

		/**
		*	Find the length of a vector
		*/
		static float vlength(const float *v);

		/**
		*	Scale a vector
		*/
		static void vscale(float *v, float div);

		/**
		*	Normalize a vector
		*/
		static void vnormal(float *v);

		/**
		*	Find dot product of a vector
		*/
		static float vdot(const float *v1, const float *v2);

		/**
		*	Add two vectors
		*/
		static void vadd(const float *src1, const float *src2, float *dst);

		/**
		*	Get an application unique Ogre::String name
		*	@param : prefix - Ogre::String that need not be unique
		*   @return : Ogre::String - in the forum of "prefix00". 00 is app independant.
		*/
		static Ogre::String GetUniqueObjName(Ogre::String prefix);

		/**
		*	Get an application unique Ogre::String name
		*	@param : prefix - Ogre::String that need not be unique
		*   @return : Ogre::String - in the forum of "prefix00". 00 is app independant.
		*/
		static Ogre::String GetUniqueMtlName(Ogre::String prefix);

		/**
		*	Get an application unique Ogre::String name
		*	@param : prefix - Ogre::String that need not be unique
		*   @return : Ogre::String - in the forum of "prefix00". 00 is app independant.
		*/
		static Ogre::String GetUniqueName(Ogre::String prefix);




    protected:

		static int				mObjCount;
		static int				mMtlCount;
		static int				mNameCount;
        static Ogre::String		mExePath;
        static unsigned int		mVertexBufferSize;
        static unsigned int		mIndexBufferSize;
        static Ogre::Vector3*	mVertexBuffer;
        static unsigned long*	mIndexBuffer;

    private:
        TemplateUtils();
        ~TemplateUtils();
};
