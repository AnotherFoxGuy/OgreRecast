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

#include <math.h>
#include "Ogre.h"
#include "tinyxml.h"

#include "GUtility.h"
#include "SharedData.h"

using namespace Ogre;

Ogre::String   TemplateUtils::mExePath = "";
unsigned int   TemplateUtils::mVertexBufferSize = 0;
unsigned int   TemplateUtils::mIndexBufferSize = 0;
Ogre::Vector3 *TemplateUtils::mVertexBuffer = 0;
unsigned long *TemplateUtils::mIndexBuffer = 0;
int			   TemplateUtils::mMtlCount = 0;
int			   TemplateUtils::mObjCount = 0;
int			   TemplateUtils::mNameCount = 0;

//-----------------------------------------------------------------------------------------
Ogre::String TemplateUtils::GetUniqueObjName(Ogre::String prefix)
{
	return prefix + Ogre::StringConverter::toString(mObjCount++);
}

//-----------------------------------------------------------------------------------------
Ogre::String TemplateUtils::GetUniqueMtlName(Ogre::String prefix)
{
	return prefix + Ogre::StringConverter::toString(mMtlCount++);
}

//-----------------------------------------------------------------------------------------
Ogre::String TemplateUtils::GetUniqueName(Ogre::String prefix)
{
	return prefix + Ogre::StringConverter::toString(mNameCount++);
}

//-----------------------------------------------------------------------------------------
void TemplateUtils::ParseStringVector(Ogre::String &str, Ogre::StringVector &list)
{
    list.clear();
    Ogre::StringUtil::trim(str,true,true);
    if(str == "") 
        return;

    int pos = str.find(";");
    while(pos != -1)
    {
        list.push_back(str.substr(0,pos));
        str.erase(0,pos + 1);
        pos = str.find(";");
    }
	
    if(str != "") 
        list.push_back(str);
}
//-----------------------------------------------------------------------------------------
void TemplateUtils::ParseUTFStringVector(Ogre::UTFString &str, UTFStringVector &list)
{   
    list.clear();
    if(str == "") 
        return;

    int pos = str.find(";");
    while(pos != -1)
    {
        list.push_back(str.substr(0,pos));
        str.erase(0,pos + 1);
        pos = str.find(";");
    }
  
    if(str != "") 
        list.push_back(str);
}
//-----------------------------------------------------------------------------------------
void TemplateUtils::CleanPath(Ogre::String &path)
{
    std::replace( path.begin(), path.end(), '\\', '/' );

    int pos = 0;
    pos = path.find("//",0);
    while(pos != -1)
    {
        path.erase(pos,1);
        pos = path.find("//",0);
    }
    
    pos = path.find("/./",0);
    while(pos != -1)
    {
        path.erase(pos,2);
        pos = path.find("/./",0);
    }
}
//-----------------------------------------------------------------------------------------
Ogre::String TemplateUtils::ExtractFileName(const Ogre::String &path)
{
    int pos1 = -1, pos2 = -1;
    Ogre::String strName = "";
    pos1 = path.find_last_of("\\");
    pos2 = path.find_last_of("/");

    int pos = std::max(pos1,pos2);

    if(pos != -1)
    {
        strName = path.substr(pos + 1,path.size() - (pos + 1));
    }

    return strName;
}
//-----------------------------------------------------------------------------------------
Ogre::String TemplateUtils::ExtractFilePath(const Ogre::String &path)
{
    int pos1 = -1, pos2 = -1;
    Ogre::String strName = "";
    pos1 = path.find_last_of("\\");
    pos2 = path.find_last_of("/");

    int pos = std::max(pos1,pos2);

    if(pos != -1)
    {
        strName = path.substr(0,pos + 1);
    }

    return strName;
}
//-----------------------------------------------------------------------------------------
Ogre::String TemplateUtils::QualifyPath(const Ogre::String &dirname)
{
    Ogre::String path = dirname;
    if(path.substr(0,1) == ".") path = mExePath + "/" + path;

    std::replace(path.begin(),path.end(),'\\','/');
    
    // Remember if there is a leading '/'
    bool leadingSlash = false;
    if(path.substr(0,1) == "/")
        leadingSlash = true;

    Ogre::StringVector list;
    int pos = path.find("/");
    while(pos != -1)
    {
        if(pos > 0 && path.substr(0,pos) != ".")  // Ignore zero strings and same directory pointers
            list.push_back(path.substr(0,pos));
        path.erase(0,pos + 1);
        pos = path.find("/");
    }

    if(path != "") 
        list.push_back(path);

    unsigned int pos2 = 0;
    while(list.size() > pos2)
    {
        if(list[pos2] == "..")
        {
              list.erase(list.begin() + pos2 - 1,list.begin() + pos2 + 1);
            pos2--;
        }
        else
            pos2++;
    }

    if(list.size() == 0) 
        return "";

    path = list[0];

    for(unsigned int i = 1;i < list.size();i++)
    { 
        path += "/" + list[i];
    }
    
    if(leadingSlash)
        path = "/" + path;

    return path;
}
//-----------------------------------------------------------------------------------------
Ogre::String TemplateUtils::GetRelativePath(const Ogre::String pathFrom,const Ogre::String pathTo)
{
    Ogre::String sFrom = QualifyPath(pathFrom);
    Ogre::String sTo = QualifyPath(pathTo);

    Ogre::String sFromFirst = sFrom.substr(0,1);
    Ogre::String sToFirst = sTo.substr(0,1);
    Ogre::StringUtil::toLowerCase(sFromFirst);
    Ogre::StringUtil::toLowerCase(sToFirst);

    if( sFromFirst != sToFirst) return sTo;

    Ogre::StringVector listfrom;
    int pos = sFrom.find("/");
    while(pos != -1)
    {
        listfrom.push_back(sFrom.substr(0,pos));
        sFrom.erase(0,pos + 1);
        pos = sFrom.find("/");
    }
    
    if(sFrom != "") 
        listfrom.push_back(sFrom);

    Ogre::StringVector listto;
    pos = sTo.find("/");
    while(pos != -1)
    {
        listto.push_back(sTo.substr(0,pos));
        sTo.erase(0,pos + 1);
        pos = sTo.find("/");
    }
    
    if(sTo != "") 
        listto.push_back(sTo);

    unsigned int length = std::min(listfrom.size(), listto.size());

    unsigned int i;
    for(i = 0;i < length;i++)
    {
        Ogre::String valFrom = listfrom[i];
        Ogre::String valTo = listto[i];
        Ogre::StringUtil::toLowerCase(valFrom);
        Ogre::StringUtil::toLowerCase(valTo);
        if(valTo != valFrom) 
            break;
    }

    listfrom.erase(listfrom.begin(),listfrom.begin() + i);
    listto.erase(listto.begin(),listto.begin() + i);

    if(listfrom.size() == 0 && listto.size() == 0) 
        return "./";

    Ogre::String strRet = "";
    for(i = 0;i < listfrom.size();i++) 
        strRet += "../";

    if(strRet.length() == 0) 
        strRet = "./";

    for(i = 0;i < listto.size();i++) 
        strRet += listto[i] + "/";

    if(listto.size())
    {
        strRet.erase(strRet.size() - 1,1);
    }
    return strRet;
}
//-----------------------------------------------------------------------------------------
void TemplateUtils::GetMeshData(const Ogre::MeshPtr mesh, size_t &vertex_count, size_t &index_count,
                               const Ogre::Vector3 &position, const Ogre::Quaternion &orient, const Ogre::Vector3 &scale)
{
    bool added_shared = false;
    size_t current_offset = 0;
    size_t shared_offset = 0;
    size_t next_offset = 0;
    size_t index_offset = 0;

    vertex_count = index_count = 0;

    // Calculate how many vertices and indices we're going to need
    for (unsigned short i = 0; i < mesh->getNumSubMeshes(); ++i)
    {
        Ogre::SubMesh* submesh = mesh->getSubMesh( i );

        // We only need to add the shared vertices once
        if(submesh->useSharedVertices)
        {
            if( !added_shared )
            {
                vertex_count += mesh->sharedVertexData->vertexCount;
                added_shared = true;
            }
        }
        else
        {
            vertex_count += submesh->vertexData->vertexCount;
        }

        // Add the indices
        index_count += submesh->indexData->indexCount;
    }


    if(vertex_count > mVertexBufferSize)
    {
        OGRE_FREE(mVertexBuffer, Ogre::MEMCATEGORY_GEOMETRY);
        mVertexBuffer = OGRE_ALLOC_T(Ogre::Vector3,vertex_count, Ogre::MEMCATEGORY_GEOMETRY);
        mVertexBufferSize = vertex_count;
    }

    if(index_count > mIndexBufferSize)
    {
        OGRE_FREE(mIndexBuffer, Ogre::MEMCATEGORY_GEOMETRY);
        mIndexBuffer = OGRE_ALLOC_T(unsigned long,index_count, Ogre::MEMCATEGORY_GEOMETRY);
        mIndexBufferSize = index_count;
    }

    added_shared = false;

    // Run through the submeshes again, adding the data into the arrays
    for ( unsigned short i = 0; i < mesh->getNumSubMeshes(); ++i)
    {
        Ogre::SubMesh* submesh = mesh->getSubMesh(i);

        Ogre::VertexData* vertex_data = submesh->useSharedVertices ? mesh->sharedVertexData : submesh->vertexData;

        if((!submesh->useSharedVertices)||(submesh->useSharedVertices && !added_shared))
        {
            if(submesh->useSharedVertices)
            {
              added_shared = true;
              shared_offset = current_offset;
            }

            const Ogre::VertexElement* posElem = vertex_data->vertexDeclaration->findElementBySemantic(Ogre::VES_POSITION);

            Ogre::HardwareVertexBufferSharedPtr vbuf = vertex_data->vertexBufferBinding->getBuffer(posElem->getSource());

            unsigned char* vertex = static_cast<unsigned char*>(vbuf->lock(Ogre::HardwareBuffer::HBL_READ_ONLY));

            // There is _no_ baseVertexPointerToElement() which takes an Ogre::Real or a double
            //  as second argument. So make it float, to avoid trouble when Ogre::Real will
            //  be comiled/typedefed as double:
            //      Ogre::Real* pReal;
            float* pReal;

            for( size_t j = 0; j < vertex_data->vertexCount; ++j, vertex += vbuf->getVertexSize())
            {
                posElem->baseVertexPointerToElement(vertex, &pReal);

                Ogre::Vector3 pt(pReal[0], pReal[1], pReal[2]);

                mVertexBuffer[current_offset + j] = (orient * (pt * scale)) + position;
            }

            vbuf->unlock();
            next_offset += vertex_data->vertexCount;
        }


        Ogre::IndexData* index_data = submesh->indexData;
        size_t numTris = index_data->indexCount / 3;
        Ogre::HardwareIndexBufferSharedPtr ibuf = index_data->indexBuffer;

        bool use32bitindexes = (ibuf->getType() == Ogre::HardwareIndexBuffer::IT_32BIT);

        unsigned long*  pLong = static_cast<unsigned long*>(ibuf->lock(Ogre::HardwareBuffer::HBL_READ_ONLY));
        unsigned short* pShort = reinterpret_cast<unsigned short*>(pLong);

        size_t offset = (submesh->useSharedVertices)? shared_offset : current_offset;

        if ( use32bitindexes )
        {
            for ( size_t k = 0; k < numTris*3; ++k)
            {
                mIndexBuffer[index_offset++] = pLong[k] + static_cast<unsigned long>(offset);
            }
        }
        else
        {
            for ( size_t k = 0; k < numTris*3; ++k)
            {
                mIndexBuffer[index_offset++] = static_cast<unsigned long>(pShort[k]) + static_cast<unsigned long>(offset);
            }
        }

        ibuf->unlock();
        current_offset = next_offset;
    }
    index_count = index_offset;
}
//-----------------------------------------------------------------------------------------
void TemplateUtils::GetMeshDataEx(const Ogre::Entity *entity, size_t &vertex_count, size_t &index_count,
                               const Ogre::Vector3 &position, const Ogre::Quaternion &orient, const Ogre::Vector3 &scale)
{
    bool added_shared = false;
    size_t current_offset = 0;
    size_t shared_offset = 0;
    size_t next_offset = 0;
    size_t index_offset = 0;

    vertex_count = index_count = 0;

    Ogre::MeshPtr mesh = entity->getMesh();

    // Calculate how many vertices and indices we're going to need
    for (unsigned short i = 0; i < mesh->getNumSubMeshes(); ++i)
    {
        Ogre::SubMesh* submesh = mesh->getSubMesh( i );

        // We only need to add the shared vertices once
        if(submesh->useSharedVertices)
        {
            if( !added_shared )
            {
                vertex_count += mesh->sharedVertexData->vertexCount;
                added_shared = true;
            }
        }
        else
        {
            vertex_count += submesh->vertexData->vertexCount;
        }

        // Add the indices
        index_count += submesh->indexData->indexCount;
    }


    if(vertex_count > mVertexBufferSize)
    {
        OGRE_FREE(mVertexBuffer, Ogre::MEMCATEGORY_GEOMETRY);
        mVertexBuffer = OGRE_ALLOC_T(Ogre::Vector3,vertex_count, Ogre::MEMCATEGORY_GEOMETRY);
        mVertexBufferSize = vertex_count;
    }

    if(index_count > mIndexBufferSize)
    {
        OGRE_FREE(mIndexBuffer, Ogre::MEMCATEGORY_GEOMETRY);
        mIndexBuffer = OGRE_ALLOC_T(unsigned long,index_count, Ogre::MEMCATEGORY_GEOMETRY);
        mIndexBufferSize = index_count;
    }

    added_shared = false;

    // Run through the submeshes again, adding the data into the arrays
    for ( unsigned short i = 0; i < mesh->getNumSubMeshes(); ++i)
    {
        if(!entity->getSubEntity(i)->isVisible())
            continue;

        Ogre::SubMesh* submesh = mesh->getSubMesh(i);

        Ogre::VertexData* vertex_data = submesh->useSharedVertices ? mesh->sharedVertexData : submesh->vertexData;

        if((!submesh->useSharedVertices)||(submesh->useSharedVertices && !added_shared))
        {
            if(submesh->useSharedVertices)
            {
              added_shared = true;
              shared_offset = current_offset;
            }

            const Ogre::VertexElement* posElem = vertex_data->vertexDeclaration->findElementBySemantic(Ogre::VES_POSITION);

            Ogre::HardwareVertexBufferSharedPtr vbuf = vertex_data->vertexBufferBinding->getBuffer(posElem->getSource());

            unsigned char* vertex = static_cast<unsigned char*>(vbuf->lock(Ogre::HardwareBuffer::HBL_READ_ONLY));

            // There is _no_ baseVertexPointerToElement() which takes an Ogre::Real or a double
            //  as second argument. So make it float, to avoid trouble when Ogre::Real will
            //  be comiled/typedefed as double:
            //      Ogre::Real* pReal;
            float* pReal;

            for( size_t j = 0; j < vertex_data->vertexCount; ++j, vertex += vbuf->getVertexSize())
            {
                posElem->baseVertexPointerToElement(vertex, &pReal);

                Ogre::Vector3 pt(pReal[0], pReal[1], pReal[2]);

                mVertexBuffer[current_offset + j] = (orient * (pt * scale)) + position;
            }

            vbuf->unlock();
            next_offset += vertex_data->vertexCount;
        }


        Ogre::IndexData* index_data = submesh->indexData;
        size_t numTris = index_data->indexCount / 3;
        Ogre::HardwareIndexBufferSharedPtr ibuf = index_data->indexBuffer;

        bool use32bitindexes = (ibuf->getType() == Ogre::HardwareIndexBuffer::IT_32BIT);

        unsigned long*  pLong = static_cast<unsigned long*>(ibuf->lock(Ogre::HardwareBuffer::HBL_READ_ONLY));
        unsigned short* pShort = reinterpret_cast<unsigned short*>(pLong);

        size_t offset = (submesh->useSharedVertices)? shared_offset : current_offset;

        if ( use32bitindexes )
        {
            for ( size_t k = 0; k < numTris*3; ++k)
            {
                mIndexBuffer[index_offset++] = pLong[k] + static_cast<unsigned long>(offset);
            }
        }
        else
        {
            for ( size_t k = 0; k < numTris*3; ++k)
            {
                mIndexBuffer[index_offset++] = static_cast<unsigned long>(pShort[k]) + static_cast<unsigned long>(offset);
            }
        }

        ibuf->unlock();
        current_offset = next_offset;
    }
    index_count = index_offset;
}

//-----------------------------------------------------------------------------------------
void TemplateUtils::getMeshInformation(const Ogre::MeshPtr mesh, size_t &vertex_count, Ogre::Vector3* &vertices,
                        size_t &index_count, unsigned long* &indices, const Ogre::Vector3 &position,
                        const Ogre::Quaternion &orient, const Ogre::Vector3 &scale)
{
    bool added_shared = false;
    size_t current_offset = 0;
    size_t shared_offset = 0;
    size_t next_offset = 0;
    size_t index_offset = 0;


    vertex_count = index_count = 0;

    // Calculate how many vertices and indices we're going to need
    for ( unsigned short i = 0; i < mesh->getNumSubMeshes(); ++i)
    {
        Ogre::SubMesh* submesh = mesh->getSubMesh(i);
        // We only need to add the shared vertices once
        if(submesh->useSharedVertices)
        {
            if( !added_shared )
            {
                vertex_count += mesh->sharedVertexData->vertexCount;
                added_shared = true;
            }
        }
        else
        {
            vertex_count += submesh->vertexData->vertexCount;
        }
        // Add the indices
        index_count += submesh->indexData->indexCount;
    }

	
    // Allocate space for the vertices and indices
    vertices = new Ogre::Vector3[vertex_count];
    indices = new unsigned long[index_count];

    added_shared = false;

    // Run through the submeshes again, adding the data into the arrays
    for (unsigned short i = 0; i < mesh->getNumSubMeshes(); ++i)
    {
        Ogre::SubMesh* submesh = mesh->getSubMesh(i);

        Ogre::VertexData* vertex_data = submesh->useSharedVertices ? mesh->sharedVertexData : submesh->vertexData;

        if ((!submesh->useSharedVertices) || (submesh->useSharedVertices && !added_shared))
        {
            if(submesh->useSharedVertices)
            {
                added_shared = true;
                shared_offset = current_offset;
            }

            const Ogre::VertexElement* posElem =
                vertex_data->vertexDeclaration->findElementBySemantic(Ogre::VES_POSITION);

            Ogre::HardwareVertexBufferSharedPtr vbuf =
                vertex_data->vertexBufferBinding->getBuffer(posElem->getSource());

            unsigned char* vertex =
                static_cast<unsigned char*>(vbuf->lock(Ogre::HardwareBuffer::HBL_READ_ONLY));

            // There is _no_ baseVertexPointerToElement() which takes an Ogre::Real or a double
            //  as second argument. So make it float, to avoid trouble when Ogre::Real will
            //  be comiled/typedefed as double:
            //Ogre::Real* pReal;
            float* pReal;

            for( size_t j = 0; j < vertex_data->vertexCount; ++j, vertex += vbuf->getVertexSize())
            {
                posElem->baseVertexPointerToElement(vertex, &pReal);
                Ogre::Vector3 pt(pReal[0], pReal[1], pReal[2]);
                vertices[current_offset + j] = (orient * (pt * scale)) + position;
            }
            
            vbuf->unlock();
            next_offset += vertex_data->vertexCount;
        }


        Ogre::IndexData* index_data = submesh->indexData;
        size_t numTris = index_data->indexCount / 3;
        Ogre::HardwareIndexBufferSharedPtr ibuf = index_data->indexBuffer;
        
        bool use32bitindexes = (ibuf->getType() == Ogre::HardwareIndexBuffer::IT_32BIT);

        unsigned long* pLong = static_cast<unsigned long*>(ibuf->lock(Ogre::HardwareBuffer::HBL_READ_ONLY));
        unsigned short* pShort = reinterpret_cast<unsigned short*>(pLong);

        size_t offset = (submesh->useSharedVertices)? shared_offset : current_offset;

        if ( use32bitindexes )
        {
            for ( size_t k = 0; k < numTris*3; ++k)
            {
                indices[index_offset++] = pLong[k] + static_cast<unsigned long>(offset);
            }
        }
        else
        {
            for ( size_t k = 0; k < numTris*3; ++k)
            {
                indices[index_offset++] = static_cast<unsigned long>(pShort[k]) +
                                          static_cast<unsigned long>(offset);
            }
        }

        ibuf->unlock();
        current_offset = next_offset;
    }
}

//-----------------------------------------------------------------------------------------
// Get the mesh information for the given mesh.
// Code found in Wiki: www.ogre3d.org/wiki/index.php/RetrieveVertexData
void TemplateUtils::getMeshInformationEX(const Ogre::MeshPtr mesh, size_t &vertex_count, Ogre::Vector3* &vertices,
                                size_t &index_count, unsigned long* &indices, const Ogre::Vector3 &position,
								const Ogre::Quaternion &orient, const Ogre::Vector3 &scale)
{
    bool added_shared = false;
    size_t current_offset = 0;
    size_t shared_offset = 0;
    size_t next_offset = 0;
    size_t index_offset = 0;

    vertex_count = index_count = 0;

    // Calculate how many vertices and indices we're going to need
    for (unsigned short i = 0; i < mesh->getNumSubMeshes(); ++i)
    {
        Ogre::SubMesh* submesh = mesh->getSubMesh( i );

        // We only need to add the shared vertices once
        if(submesh->useSharedVertices)
        {
            if( !added_shared )
            {
                vertex_count += mesh->sharedVertexData->vertexCount;
                added_shared = true;
            }
        }
        else
        {
            vertex_count += submesh->vertexData->vertexCount;
        }

        // Add the indices
        index_count += submesh->indexData->indexCount;
    }


    // Allocate space for the vertices and indices
    vertices = new Ogre::Vector3[vertex_count];
    indices = new unsigned long[index_count];

    added_shared = false;

    // Run through the submeshes again, adding the data into the arrays
    for ( unsigned short i = 0; i < mesh->getNumSubMeshes(); ++i)
    {
        Ogre::SubMesh* submesh = mesh->getSubMesh(i);

        Ogre::VertexData* vertex_data = submesh->useSharedVertices ? mesh->sharedVertexData : submesh->vertexData;

        if((!submesh->useSharedVertices)||(submesh->useSharedVertices && !added_shared))
        {
            if(submesh->useSharedVertices)
            {
                added_shared = true;
                shared_offset = current_offset;
            }

            const Ogre::VertexElement* posElem =
                vertex_data->vertexDeclaration->findElementBySemantic(Ogre::VES_POSITION);

            Ogre::HardwareVertexBufferSharedPtr vbuf =
                vertex_data->vertexBufferBinding->getBuffer(posElem->getSource());

            unsigned char* vertex =
                static_cast<unsigned char*>(vbuf->lock(Ogre::HardwareBuffer::HBL_READ_ONLY));

            // There is _no_ baseVertexPointerToElement() which takes an Ogre::Real or a double
            //  as second argument. So make it float, to avoid trouble when Ogre::Real will
            //  be comiled/typedefed as double:
            //      Ogre::Real* pReal;
            float* pReal;

            for( size_t j = 0; j < vertex_data->vertexCount; ++j, vertex += vbuf->getVertexSize())
            {
                posElem->baseVertexPointerToElement(vertex, &pReal);

                Ogre::Vector3 pt(pReal[0], pReal[1], pReal[2]);

                vertices[current_offset + j] = (orient * (pt * scale)) + position;
            }

            vbuf->unlock();
            next_offset += vertex_data->vertexCount;
        }


        Ogre::IndexData* index_data = submesh->indexData;
        size_t numTris = index_data->indexCount / 3;
        Ogre::HardwareIndexBufferSharedPtr ibuf = index_data->indexBuffer;

        bool use32bitindexes = (ibuf->getType() == Ogre::HardwareIndexBuffer::IT_32BIT);

        unsigned long*  pLong = static_cast<unsigned long*>(ibuf->lock(Ogre::HardwareBuffer::HBL_READ_ONLY));
        unsigned short* pShort = reinterpret_cast<unsigned short*>(pLong);


        size_t offset = (submesh->useSharedVertices)? shared_offset : current_offset;

        // Ogre 1.6 patch (commenting the static_cast...) - index offsets start from 0 for each submesh
        if ( use32bitindexes )
        {
            for ( size_t k = 0; k < numTris*3; ++k)
            {
                indices[index_offset++] = pLong[k] /*+ static_cast<unsigned long>(offset)*/;
            }
        }
        else
        {
            for ( size_t k = 0; k < numTris*3; ++k)
            {
                indices[index_offset++] = static_cast<unsigned long>(pShort[k]) /*+
                    static_cast<unsigned long>(offset)*/;
            }
        }

        ibuf->unlock();
        current_offset = next_offset;
    }
} 

//-----------------------------------------------------------------------------------------
void TemplateUtils::getMeshInformationEXA(Ogre::Entity &entity, size_t &vertex_count, Ogre::Vector3* &vertices,
                                size_t &index_count, unsigned long* &indices, const Ogre::Vector3 &position,
                                const Ogre::Quaternion &orient, const Ogre::Vector3 &scale)
{
    bool added_shared = false;
    size_t current_offset = 0;
    size_t shared_offset = 0;
    size_t next_offset = 0;
    size_t index_offset = 0;
    vertex_count = index_count = 0;

   Ogre::MeshPtr mesh = entity.getMesh();


   bool useSoftwareBlendingVertices = entity.hasSkeleton();

   if (useSoftwareBlendingVertices)
   {
      entity._updateAnimation();
   }

    // Calculate how many vertices and indices we're going to need
    for (unsigned short i = 0; i < mesh->getNumSubMeshes(); ++i)
    {
        Ogre::SubMesh* submesh = mesh->getSubMesh( i );

        // We only need to add the shared vertices once
        if(submesh->useSharedVertices)
        {
            if( !added_shared )
            {
                vertex_count += mesh->sharedVertexData->vertexCount;
                added_shared = true;
            }
        }
        else
        {
            vertex_count += submesh->vertexData->vertexCount;
        }

        // Add the indices
        index_count += submesh->indexData->indexCount;
    }


    // Allocate space for the vertices and indices
    vertices = new Ogre::Vector3[vertex_count];
    indices = new unsigned long[index_count];

    added_shared = false;

    // Run through the submeshes again, adding the data into the arrays
    for ( unsigned short i = 0; i < mesh->getNumSubMeshes(); ++i)
    {
        Ogre::SubMesh* submesh = mesh->getSubMesh(i);

      //----------------------------------------------------------------
      // GET VERTEXDATA
      //----------------------------------------------------------------

        //Ogre::VertexData* vertex_data = submesh->useSharedVertices ? mesh->sharedVertexData : submesh->vertexData;
      Ogre::VertexData* vertex_data;

      //When there is animation:
      if(useSoftwareBlendingVertices)
#ifdef BUILD_AGAINST_AZATHOTH
         vertex_data = submesh->useSharedVertices ? entity._getSharedBlendedVertexData() : entity.getSubEntity(i)->_getBlendedVertexData();
#else
         vertex_data = submesh->useSharedVertices ? entity._getSkelAnimVertexData() : entity.getSubEntity(i)->_getSkelAnimVertexData();
#endif
      else
         vertex_data = submesh->useSharedVertices ? mesh->sharedVertexData : submesh->vertexData;


        if((!submesh->useSharedVertices)||(submesh->useSharedVertices && !added_shared))
        {
            if(submesh->useSharedVertices)
            {
                added_shared = true;
                shared_offset = current_offset;
            }

            const Ogre::VertexElement* posElem =
                vertex_data->vertexDeclaration->findElementBySemantic(Ogre::VES_POSITION);

            Ogre::HardwareVertexBufferSharedPtr vbuf =
                vertex_data->vertexBufferBinding->getBuffer(posElem->getSource());

            unsigned char* vertex =
                static_cast<unsigned char*>(vbuf->lock(Ogre::HardwareBuffer::HBL_READ_ONLY));

            // There is _no_ baseVertexPointerToElement() which takes an Ogre::Real or a double
            //  as second argument. So make it float, to avoid trouble when Ogre::Real will
            //  be comiled/typedefed as double:
            //      Ogre::Real* pReal;
            float* pReal;

            for( size_t j = 0; j < vertex_data->vertexCount; ++j, vertex += vbuf->getVertexSize())
            {
                posElem->baseVertexPointerToElement(vertex, &pReal);

                Ogre::Vector3 pt(pReal[0], pReal[1], pReal[2]);

                vertices[current_offset + j] = (orient * (pt * scale)) + position;
            }

            vbuf->unlock();
            next_offset += vertex_data->vertexCount;
        }


        Ogre::IndexData* index_data = submesh->indexData;
        size_t numTris = index_data->indexCount / 3;
        Ogre::HardwareIndexBufferSharedPtr ibuf = index_data->indexBuffer;

        bool use32bitindexes = (ibuf->getType() == Ogre::HardwareIndexBuffer::IT_32BIT);

        unsigned long*  pLong = static_cast<unsigned long*>(ibuf->lock(Ogre::HardwareBuffer::HBL_READ_ONLY));
        unsigned short* pShort = reinterpret_cast<unsigned short*>(pLong);


        size_t offset = (submesh->useSharedVertices)? shared_offset : current_offset;

        // Ogre 1.6 patch (commenting the static_cast...) - index offsets start from 0 for each submesh
        if ( use32bitindexes )
        {
            for ( size_t k = 0; k < numTris*3; ++k)
            {
                indices[index_offset++] = pLong[k] /*+ static_cast<unsigned long>(offset)*/;
            }
        }
        else
        {
            for ( size_t k = 0; k < numTris*3; ++k)
            {
                indices[index_offset++] = static_cast<unsigned long>(pShort[k]) /*+
                    static_cast<unsigned long>(offset)*/;
            }
        }

        ibuf->unlock();
        current_offset = next_offset;
    }
} 


//-----------------------------------------------------------------------------------------
bool TemplateUtils::PickEntity(Ogre::RaySceneQuery* mRaySceneQuery, Ogre::Ray &ray, Ogre::Entity **result, Ogre::Vector3 &hitpoint, const Ogre::String& excludeobject, Ogre::Real max_distance)
{
    mRaySceneQuery->setRay(ray);
	mRaySceneQuery->setQueryMask(QUERYFLAG_MOVABLE);
    mRaySceneQuery->setSortByDistance(true);

	unsigned int mVisibilityMask = SharedData::getSingleton().iSceneMgr->getVisibilityMask();
    
    if (mRaySceneQuery->execute().size() <= 0) return (false);

    // at this point we have raycast to a series of different objects bounding boxes.
    // we need to test these different objects to see which is the first polygon hit.
    // there are some minor optimizations (distance based) that mean we wont have to
    // check all of the objects most of the time, but the worst case scenario is that
    // we need to test every triangle of every object.
    Ogre::Real closest_distance = max_distance;
    Ogre::Vector3 closest_result;
    Ogre::RaySceneQueryResult &query_result = mRaySceneQuery->getLastResults();

    for (size_t qr_idx = 0; qr_idx < query_result.size(); qr_idx++)
    {
        // stop checking if we have found a raycast hit that is closer
        // than all remaining entities
        if ((closest_distance >= 0.0f) && (closest_distance < query_result[qr_idx].distance))
            break;
    
        // only check this result if its a hit against an entity
        if ((query_result[qr_idx].movable != NULL) && (query_result[qr_idx].movable->getMovableType().compare("Entity") == 0))
        {
            // get the entity to check
            Ogre::Entity *pentity = static_cast<Ogre::Entity*>(query_result[qr_idx].movable);

            if(!(pentity->getVisibilityFlags() & mVisibilityMask))
                continue;

            if(!pentity->getVisible() || (pentity->getName() == excludeobject)) 
                continue;

            // mesh data to retrieve
            size_t vertex_count;
            size_t index_count;

            // get the mesh information
            GetMeshData(pentity->getMesh(), vertex_count, index_count, 
                        pentity->getParentNode()->_getDerivedPosition(),
                        pentity->getParentNode()->_getDerivedOrientation(),
                        pentity->getParentNode()->_getDerivedScale());

            // test for hitting individual triangles on the mesh
            bool new_closest_found = false;
            for (int i = 0; i < static_cast<int>(index_count); i += 3)
            {
                // check for a hit against this triangle
                std::pair<bool, Ogre::Real> hit = Ogre::Math::intersects(ray, mVertexBuffer[mIndexBuffer[i]],
                         mVertexBuffer[mIndexBuffer[i+1]], mVertexBuffer[mIndexBuffer[i+2]], true, false);

                // if it was a hit check if its the closest
                if (hit.first)
                {
                    if ((closest_distance < 0.0f) || (hit.second < closest_distance))
                    {
                        // this is the closest so far, save it off
                        closest_distance = hit.second;
                        new_closest_found = true;
                    }
                }
            }

            // if we found a new closest raycast for this object, update the
            // closest_result before moving on to the next object.
            if (new_closest_found)
            {
                closest_result = ray.getPoint(closest_distance);
               // (*result) = pentity;
            }
        }
    }

    // return the result
    if (closest_distance != max_distance)
    {
        hitpoint = closest_result;
        return (true);
    }
    else
    {
        // raycast failed
        return (false);
    }
}
//-----------------------------------------------------------------------------------------
bool TemplateUtils::PickEntity(Ogre::RaySceneQuery* mRaySceneQuery, Ogre::Ray &ray, Ogre::Entity **result, Ogre::Vector3 &hitpoint, const Ogre::StringVector& excludeobjects, Ogre::Real max_distance)
{
    mRaySceneQuery->setRay(ray);
    mRaySceneQuery->setQueryMask(QUERYFLAG_MOVABLE);
    mRaySceneQuery->setSortByDistance(true);

    unsigned int mVisibilityMask = SharedData::getSingleton().iSceneMgr->getVisibilityMask();

    if (mRaySceneQuery->execute().size() <= 0) return (false);

    // at this point we have raycast to a series of different objects bounding boxes.
    // we need to test these different objects to see which is the first polygon hit.
    // there are some minor optimizations (distance based) that mean we wont have to
    // check all of the objects most of the time, but the worst case scenario is that
    // we need to test every triangle of every object.
    Ogre::Real closest_distance = max_distance;
    Ogre::Vector3 closest_result;
    Ogre::RaySceneQueryResult &query_result = mRaySceneQuery->getLastResults();
    
    for (size_t qr_idx = 0; qr_idx < query_result.size(); qr_idx++)
    {
        // stop checking if we have found a raycast hit that is closer
        // than all remaining entities
        if ((closest_distance >= 0.0f) && (closest_distance < query_result[qr_idx].distance))
        {
            break;
        }

        // only check this result if its a hit against an entity
        if ((query_result[qr_idx].movable != NULL) && (query_result[qr_idx].movable->getMovableType().compare("Entity") == 0))
        {
            // get the entity to check
            Ogre::Entity *pentity = static_cast<Ogre::Entity*>(query_result[qr_idx].movable);

            if(!(pentity->getVisibilityFlags() & mVisibilityMask))
                continue;

            if(!pentity->getVisible()) 
                continue;

            Ogre::String pname = pentity->getName();

            bool foundinlist = false;
            for(unsigned int lit = 0;lit < excludeobjects.size();lit++)
            {
                if(excludeobjects[lit] == pname)
                {
                    foundinlist = true;
                    break;
                }
            }
            
            if(foundinlist)
                continue;

            // mesh data to retrieve
            size_t vertex_count;
            size_t index_count;

            // get the mesh information
            GetMeshData(pentity->getMesh(), vertex_count, index_count, 
                        pentity->getParentNode()->_getDerivedPosition(),
                        pentity->getParentNode()->_getDerivedOrientation(),
                        pentity->getParentNode()->_getDerivedScale());

            // test for hitting individual triangles on the mesh
            bool new_closest_found = false;
            for (int i = 0; i < static_cast<int>(index_count); i += 3)
            {
                // check for a hit against this triangle
                std::pair<bool, Ogre::Real> hit = Ogre::Math::intersects(ray, mVertexBuffer[mIndexBuffer[i]],
                         mVertexBuffer[mIndexBuffer[i+1]], mVertexBuffer[mIndexBuffer[i+2]], true, false);

                // if it was a hit check if its the closest
                if (hit.first)
                {
                    if ((closest_distance < 0.0f) || (hit.second < closest_distance))
                    {
                        // this is the closest so far, save it off
                        closest_distance = hit.second;
                        new_closest_found = true;
                    }
                }
            }

            // if we found a new closest raycast for this object, update the
            // closest_result before moving on to the next object.
            if (new_closest_found)
            {
                closest_result = ray.getPoint(closest_distance);
                (*result) = pentity;
            }
        }
    }

    // return the result
    if (closest_distance != max_distance)
    {
        hitpoint = closest_result;
        return (true);
    }
    else
    {
        // raycast failed
        return (false);
    }
}
//-----------------------------------------------------------------------------------------
int TemplateUtils::PickSubMesh(Ogre::Ray& ray, Ogre::Entity* pEntity)
{
    // at this point we have raycast to a series of different objects bounding boxes.
    // we need to test these different objects to see which is the first polygon hit.
    // there are some minor optimizations (distance based) that mean we wont have to
    // check all of the objects most of the time, but the worst case scenario is that
    // we need to test every triangle of every object.
    Ogre::Real closest_distance = -1.0f;
    int closest_index = -1;
    Ogre::Vector3 closest_result;
    int closest_submesh = -1;

    // mesh data to retrieve
    size_t vertex_count;
    size_t index_count;

    // get the mesh information
    GetMeshData(pEntity->getMesh(), vertex_count, index_count,
                pEntity->getParentNode()->_getDerivedPosition(),
                pEntity->getParentNode()->_getDerivedOrientation(),
                pEntity->getParentNode()->_getDerivedScale());

    // test for hitting individual triangles on the mesh
    for (int i = 0; i < static_cast<int>(index_count); i += 3)
    {
        // check for a hit against this triangle
        std::pair<bool, Ogre::Real> hit = Ogre::Math::intersects(ray, mVertexBuffer[mIndexBuffer[i]],
                 mVertexBuffer[mIndexBuffer[i+1]], mVertexBuffer[mIndexBuffer[i+2]], true, false);

        // if it was a hit check if its the closest
        if (hit.first)
        {
            if ((closest_distance < 0.0f) || (hit.second < closest_distance))
            {
                // this is the closest so far, save it off
                closest_distance = hit.second;
                closest_index = i;
            }
        }
    }
	
    if(closest_index > -1)
    {
        int index_pos = 0;
        for (unsigned short sm = 0; sm < pEntity->getMesh()->getNumSubMeshes(); ++sm)
        {
            index_pos += pEntity->getMesh()->getSubMesh( sm )->indexData->indexCount;
            if(closest_index < index_pos)
                return sm;
        }
    }
    return -1;
}
//-----------------------------------------------------------------------------------------
bool TemplateUtils::WorldIntersect(Ogre::RaySceneQuery* mRaySceneQuery, Ogre::Ray &ray, Ogre::Vector3 &hitposition)
{
    mRaySceneQuery->setRay(ray);
    mRaySceneQuery->setQueryTypeMask(Ogre::SceneManager::ENTITY_TYPE_MASK);
    //mRaySceneQuery->setsetWorldFragmentType(Ogre::SceneQuery::WFT_SINGLE_INTERSECTION);
    Ogre::RaySceneQueryResult& qryResult = mRaySceneQuery->execute();
    Ogre::RaySceneQueryResult::iterator i = qryResult.begin();
    if (i != qryResult.end() && i->worldFragment)
    {
        hitposition = i->worldFragment->singleIntersection;
        return true;
    }
    return false;
}

//-----------------------------------------------------------------------------------------
void TemplateUtils::vzero(float *v)
{
    v[0] = 0.0;
    v[1] = 0.0;
    v[2] = 0.0;
}

//-----------------------------------------------------------------------------------------
void TemplateUtils::vset(float *v, float x, float y, float z)
{
    v[0] = x;
    v[1] = y;
    v[2] = z;
}

//-----------------------------------------------------------------------------------------
void TemplateUtils::vsub(const float *src1, const float *src2, float *dst)
{
    dst[0] = src1[0] - src2[0];
    dst[1] = src1[1] - src2[1];
    dst[2] = src1[2] - src2[2];
}

//-----------------------------------------------------------------------------------------
void TemplateUtils::vcopy(float *v1, const float *v2)
{
    int i;
    for (i = 0 ; i < 3 ; ++i)
        v1[i] = v2[i];
}

//-----------------------------------------------------------------------------------------
void TemplateUtils::vcross(const float *v1, const float *v2, float *cross)
{
    float temp[3];

    temp[0] = (v1[1] * v2[2]) - (v1[2] * v2[1]);
    temp[1] = (v1[2] * v2[0]) - (v1[0] * v2[2]);
    temp[2] = (v1[0] * v2[1]) - (v1[1] * v2[0]);
    vcopy(temp, cross);
}

//-----------------------------------------------------------------------------------------
float TemplateUtils::vlength(const float *v)
{
    return sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

//-----------------------------------------------------------------------------------------
void TemplateUtils::vscale(float *v, float div)
{
    v[0] *= div;
    v[1] *= div;
    v[2] *= div;
}

//-----------------------------------------------------------------------------------------
void TemplateUtils::vnormal(float *v)
{
    vscale(v,1.0f/vlength(v));
}

//-----------------------------------------------------------------------------------------
float TemplateUtils::vdot(const float *v1, const float *v2)
{
    return v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
}

//-----------------------------------------------------------------------------------------
void TemplateUtils::vadd(const float *src1, const float *src2, float *dst)
{
    dst[0] = src1[0] + src2[0];
    dst[1] = src1[1] + src2[1];
    dst[2] = src1[2] + src2[2];
}


//----------------------------------------------------------------------------------------
// REMOVED UNUSED/UNNEEDED OGITOR METHODS, DECLAERATION IS HERE STILL
//----------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------
Ogre::String TemplateUtils::GetValueString( /*Ogre::OgOgreProperty& value*/ )
{
	// REMOVED DUE TO LACK OF USE IN CURRENT CONTEXT 
    return "";
}
//----------------------------------------------------------------------------------------
Ogre::String TemplateUtils::GetCustomPropertySaveString(/*OgitorsCustomPropertySet *set,*/  int indentation)
{
    // REMOVED DUE TO LACK OF USE IN CURRENT CONTEXT
	return "";
}
//----------------------------------------------------------------------------------------
void TemplateUtils::ReadCustomPropertySet(TiXmlElement *element /*, OgitorsCustomPropertySet *set*/ )
{
   // REMOVED DUE TO LACK OF USE IN CURRENT CONTEXT
}
//----------------------------------------------------------------------------------------
Ogre::String TemplateUtils::GetUserDataSaveString(/*OgitorsCustomPropertySet *set,*/ int indentation)
{
   // REMOVED DUE TO LACK OF USE IN CURRENT CONTEXT
	return "";
}
//----------------------------------------------------------------------------------------
Ogre::String TemplateUtils::GetObjectSaveStringV2(/*CBaseEditor *object,*/ int indentation, bool useobjid, bool addparent)
{
	// REMOVED DUE TO LACK OF USE IN CURRENT CONTEXT
	return "";
}
//----------------------------------------------------------------------------------------
