// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2018
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// File Version: 3.0.2 (2018/02/17)


#include "Core/Logger/Logger.h"

#include "GLSLReflection.h"
#include <fstream>


GLSLReflection::GLSLReflection(GLuint handle)
    :
    mHandle(handle)
{
    eastl::string vendor(reinterpret_cast<char const*>(glGetString(GL_VENDOR)));
    mVendorIsIntel = (vendor == "Intel");
    mShaderTypeMap.insert(eastl::make_pair(GL_VERTEX_SHADER, 3));
    mShaderTypeMap.insert(eastl::make_pair(GL_GEOMETRY_SHADER, 4));
    mShaderTypeMap.insert(eastl::make_pair(GL_FRAGMENT_SHADER, 5));
    mShaderTypeMap.insert(eastl::make_pair(GL_COMPUTE_SHADER, 6));
    mShaderTypeMap.insert(eastl::make_pair(GL_TESS_CONTROL_SHADER, 7));
    mShaderTypeMap.insert(eastl::make_pair(GL_TESS_EVALUATION_SHADER, 8));

    if (mHandle > 0)
    {
        ReflectProgramInputs();
        ReflectProgramOutputs();
        ReflectDataBlocks(GL_UNIFORM_BLOCK, mUniformBlocks);
        ReflectUniforms();
        ReflectDataBlocks(GL_SHADER_STORAGE_BLOCK, mShaderStorageBlocks);
        ReflectAtomicCounterBuffers();
        ReflectSubroutines(GL_VERTEX_SUBROUTINE, mVertexSubroutines);
        ReflectSubroutines(GL_GEOMETRY_SUBROUTINE, mGeometrySubroutines);
        ReflectSubroutines(GL_FRAGMENT_SUBROUTINE, mPixelSubroutines);
        ReflectSubroutines(GL_COMPUTE_SUBROUTINE, mComputeSubroutines);
        ReflectSubroutines(GL_TESS_CONTROL_SUBROUTINE,
            mTessControlSubroutines);
        ReflectSubroutines(GL_TESS_EVALUATION_SUBROUTINE,
            mTessEvaluationSubroutines);
        ReflectSubroutineUniforms(GL_VERTEX_SUBROUTINE_UNIFORM,
            mVertexSubroutineUniforms);
        ReflectSubroutineUniforms(GL_GEOMETRY_SUBROUTINE_UNIFORM,
            mGeometrySubroutineUniforms);
        ReflectSubroutineUniforms(GL_FRAGMENT_SUBROUTINE_UNIFORM,
            mPixelSubroutineUniforms);
        ReflectSubroutineUniforms(GL_COMPUTE_SUBROUTINE_UNIFORM,
            mComputeSubroutineUniforms);
        ReflectSubroutineUniforms(GL_TESS_CONTROL_SUBROUTINE_UNIFORM,
            mTessControlSubroutineUniforms);
        ReflectSubroutineUniforms(GL_TESS_EVALUATION_SUBROUTINE_UNIFORM,
            mTessEvaluationSubroutineUniforms);
        ReflectBufferVariables();
        ReflectTransformFeedbackVaryings();
        ReflectTransformFeedbackBuffers();
    }
    else
    {
        LogError("The program handle is invalid.");
    }
}

void GLSLReflection::GetComputeShaderWorkGroupSize(GLint &numXThreads, GLint& numYThreads, GLint& numZThreads) const
{
    GLint workGroupSize[3];
    glGetProgramiv(mHandle, GL_COMPUTE_WORK_GROUP_SIZE, workGroupSize);
    numXThreads = workGroupSize[0];
    numYThreads = workGroupSize[1];
    numZThreads = workGroupSize[2];
}

void GLSLReflection::Print(std::ofstream& ostr) const
{
    // TODO: need some type of pre-amble

    ostr << "Description:" << std::endl;
    ostr << "OpenGL version = " << glGetString(GL_VERSION) << std::endl;
    ostr << "GLSL version = " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    ostr << "Vendor = " << glGetString(GL_VENDOR) << std::endl;
    ostr << "Renderer = " << glGetString(GL_RENDERER) << std::endl;
    ostr << std::endl;

    ostr << "General:" << std::endl;
    ostr << "num inputs = " << mInputs.size() << std::endl;
    ostr << "num outputs = " << mOutputs.size() << std::endl;
    ostr << "num uniform blocks = " << mUniformBlocks.size() << std::endl;
    ostr << "num atomic counter buffers = " << mAtomicCounterBuffers.size() << std::endl;
    ostr << "num uniforms = " << mUniforms.size() << std::endl;
    ostr << "num shader storage blocks = " << mShaderStorageBlocks.size() << std::endl;
    ostr << "num buffer variables = " << mBufferVariables.size() << std::endl;
    ostr << std::endl;

    for (unsigned i = 0; i < mInputs.size(); ++i)
    {
        auto const& input = mInputs[i];

        ostr << "Input[" << i << "]:" << std::endl;
        ostr << "name = " << input.name.c_str() << std::endl;
        ostr << "type = " << GetEnumName(input.type).c_str() << std::endl;
        ostr << "shader type = " << GetEnumShaderName(input.type).c_str() << std::endl;
        ostr << "location = " << input.location << std::endl;
        ostr << "array Size = " << input.arraySize << std::endl;
        ostr << "referenced by shaders = " << GetReferencedByShaderList(input.referencedBy).c_str() << std::endl;
        ostr << "is per patch = " << input.isPerPatch << std::endl;
        ostr << "location component = " << input.locationComponent << std::endl;
        ostr << std::endl;
    }

    for (unsigned i = 0; i < mOutputs.size(); ++i)
    {
        auto const& output = mOutputs[i];

        ostr << "Output[" << i << "]:" << std::endl;
        ostr << "name = " << output.name.c_str() << std::endl;
        ostr << "type = " << GetEnumName(output.type).c_str() << std::endl;
        ostr << "shader type = " << GetEnumShaderName(output.type).c_str() << std::endl;
        ostr << "location = " << output.location << std::endl;
        ostr << "array Size = " << output.arraySize << std::endl;
        ostr << "referenced by shaders = " << GetReferencedByShaderList(output.referencedBy).c_str() << std::endl;
        ostr << "is per patch = " << output.isPerPatch << std::endl;
        ostr << "location component = " << output.locationIndex << std::endl;
        ostr << "location index = " << output.locationIndex << std::endl;
        ostr << std::endl;
    }

    for (unsigned i = 0; i < mUniformBlocks.size(); ++i)
    {
        auto const& block = mUniformBlocks[i];

        ostr << "UniformBlock[" << i << "]:" << std::endl;
        ostr << "name = " << block.name.c_str() << std::endl;
        ostr << "buffer binding = " << block.bufferBinding << std::endl;
        ostr << "buffer data size = " << block.bufferDataSize << std::endl;
        ostr << "referenced by shaders = " << GetReferencedByShaderList(block.referencedBy).c_str() << std::endl;
        ostr << "active variables = " << block.activeVariables.size() << std::endl;

        // Format the uniform block and its data members to look like a declaration
        // in the shader code with helpful comments.
        ostr << "declaration = " << std::endl;
        ostr << "  uniform " << block.name.c_str() << std::endl;
        ostr << "  {" << std::endl;
        for (unsigned v = 0; v < block.activeVariables.size(); ++v)
        {
            auto const& uniform = mUniforms[block.activeVariables[v]];

            ostr << "      " << GetEnumShaderName(uniform.type).c_str() << " " << uniform.name.c_str();
            if (uniform.arraySize > 1)
            {
                ostr << "[" << uniform.arraySize << "]";
            }
            ostr << "; //";
            ostr << " offset=" << uniform.offset;
            ostr << " size=" << GetEnumSize(uniform.type, uniform.arraySize,
                uniform.arrayStride, uniform.matrixStride, uniform.isRowMajor);
            if (uniform.arrayStride > 0)
            {
                ostr << " arrayStride=" << uniform.arrayStride;
            }
            if (uniform.matrixStride > 0)
            {
                ostr << " matrixStride=" << uniform.matrixStride;
                ostr << " rowMajor=" << uniform.isRowMajor;
            }
            ostr << std::endl;
        }
        ostr << "  };" << std::endl;
        ostr << std::endl;
    }

    for (unsigned i = 0; i < mAtomicCounterBuffers.size(); ++i)
    {
        auto const& acBuffer = mAtomicCounterBuffers[i];

        ostr << "AtomicCounterBuffer[" << i << "]:" << std::endl;
        ostr << "buffer binding = " << acBuffer.bufferBinding << std::endl;
        ostr << "buffer data size = " << acBuffer.bufferDataSize << std::endl;
        ostr << "referenced by shaders = " << GetReferencedByShaderList(acBuffer.referencedBy).c_str() << std::endl;
        ostr << "active variables = " << acBuffer.activeVariables.size() << std::endl;
        ostr << std::endl;
    }

    for (unsigned i = 0; i < mUniforms.size(); ++i)
    {
        auto const& uniform = mUniforms[i];

        ostr << "Uniform[" << i << "]:" << std::endl;
        ostr << "name = " << uniform.fullName.c_str() << std::endl;
        ostr << "type = " << GetEnumName(uniform.type).c_str() << std::endl;
        ostr << "shader type = " << GetEnumShaderName(uniform.type).c_str() << std::endl;
        ostr << "referenced by shaders = " << GetReferencedByShaderList(uniform.referencedBy).c_str() << std::endl;

        // In a uniform block.
        if (uniform.blockIndex >= 0)
        {
            ostr << "array size = " << uniform.arraySize << std::endl;
            ostr << "offset = " << uniform.offset << std::endl;
            ostr << "uniform block index = " << uniform.blockIndex << std::endl;
            ostr << "array stride = " << uniform.arrayStride << std::endl;
            ostr << "matrix stride = " << uniform.matrixStride << std::endl;
            ostr << "is row major = " << uniform.isRowMajor << std::endl;
            ostr << "size=" << GetEnumSize(uniform.type, uniform.arraySize,
                uniform.arrayStride, uniform.matrixStride, uniform.isRowMajor) << std::endl;
        }

        // Atomic counter.
        else if (uniform.atomicCounterBufferIndex >= 0)
        {
            ostr << "atomic counter buffer index = " << uniform.atomicCounterBufferIndex << std::endl;

            auto const& acBuffer = mAtomicCounterBuffers[uniform.atomicCounterBufferIndex];
            ostr << "declaration = " << std::endl;
            ostr << "  layout(binding = " << acBuffer.bufferBinding << ", offset = " << uniform.offset;
            ostr << ") uniform " << GetEnumShaderName(uniform.type).c_str() << " " << uniform.name.c_str() << ";" << std::endl;
        }

        // Opaque type is not in a uniform block.
        else
        {
            ostr << "declaration = " << std::endl;
            ostr << "  uniform " << GetEnumShaderName(uniform.type).c_str() << " " << uniform.name.c_str() << ";" << std::endl;
        }

        ostr << std::endl;
    }

    for (unsigned i = 0; i < mShaderStorageBlocks.size(); ++i)
    {
        auto const& block = mShaderStorageBlocks[i];

        ostr << "ShaderStorageBlock[" << i << "]:" << std::endl;
        ostr << "name = " << block.name.c_str() << std::endl;
        ostr << "buffer binding = " << block.bufferBinding << std::endl;
        ostr << "buffer data size = " << block.bufferDataSize << std::endl;
        ostr << "referenced by shaders = " << GetReferencedByShaderList(block.referencedBy).c_str() << std::endl;
        ostr << "active variables = " << block.activeVariables.size() << std::endl;

        // Format the buffer block and its data members to look like a declaration
        // in the shader code with helpful comments.
        ostr << "declaration = " << std::endl;
        ostr << "  buffer " << block.name.c_str() << std::endl;
        ostr << "  {" << std::endl;
        GLint topLevelArrayStride = 0;
        GLint topLevelArrayOffset = 0;
        for (unsigned v = 0; v < block.activeVariables.size(); ++v)
        {
            auto const& bufferVar = mBufferVariables[block.activeVariables[v]];

            if (bufferVar.topLevelArrayStride != topLevelArrayStride)
            {
                if (topLevelArrayStride > 0)
                {
                    ostr << "      " << "} [];" << std::endl;
                }
                else
                {
                    ostr << "      struct //";
                    ostr << " offset=" << bufferVar.offset;
                    ostr << " structSize=" << bufferVar.topLevelArrayStride;
                    ostr << std::endl;
                    ostr << "      {" << std::endl;
                }
                topLevelArrayStride = bufferVar.topLevelArrayStride;
                topLevelArrayOffset = bufferVar.offset;
            }

            // Generate what the declaration for this variable might look like.
            if (topLevelArrayStride > 0)
            {
                ostr << "    ";
            }
            ostr << "      " << GetEnumShaderName(bufferVar.type).c_str() << " " << bufferVar.name.c_str();
            if (bufferVar.arrayStride > 0)
            {
                auto arraySize = bufferVar.arraySize;
                if (0 == arraySize)
                {
                    // Guess array size based on offset for next variable in struct.
                    GLint nextOffset = block.bufferDataSize;
                    if ((v+1) < block.activeVariables.size())
                    {
                        nextOffset = mBufferVariables[block.activeVariables[v+1]].offset;
                    }
                    arraySize = (nextOffset - bufferVar.offset) / bufferVar.arrayStride;
                }
                ostr << '[' << arraySize << ']';
            }

            ostr << "; //";
            auto const size = GetEnumSize(bufferVar.type, bufferVar.arraySize,
                bufferVar.arrayStride, bufferVar.matrixStride, bufferVar.isRowMajor);
            if (size > 0)
            {
                ostr << " size=" << size;
            }
            if (topLevelArrayStride > 0)
            {
                ostr << " structOffset=" << (bufferVar.offset - topLevelArrayOffset);
            }
            ostr << " bufferOffset=" << bufferVar.offset;
            if (bufferVar.arrayStride > 0)
            {
                ostr << " arrayStride=" << bufferVar.arrayStride;
            }
            if (bufferVar.matrixStride > 0)
            {
                ostr << " matrixStride=" << bufferVar.matrixStride;
                ostr << " rowMajor=" << bufferVar.isRowMajor;
            }
            ostr << std::endl;
        }
        if (topLevelArrayStride > 0)
        {
            ostr << "      " << "} [];" << std::endl;
        }
        ostr << "  };" << std::endl;
        ostr << std::endl;
    }

    for (unsigned i = 0; i < mBufferVariables.size(); ++i)
    {
        auto const& bufferVar = mBufferVariables[i];

        ostr << "BufferVariable[" << i << "]:" << std::endl;
        ostr << "name = " << bufferVar.fullName.c_str() << std::endl;
        ostr << "type = " << GetEnumName(bufferVar.type).c_str() << std::endl;
        ostr << "shader type = " << GetEnumShaderName(bufferVar.type).c_str() << std::endl;
        ostr << "referenced by shaders = " << GetReferencedByShaderList(bufferVar.referencedBy).c_str() << std::endl;

        // In a buffer block.
        if (bufferVar.blockIndex >= 0)
        {
            ostr << "array size = " << bufferVar.arraySize << std::endl;
            ostr << "offset = " << bufferVar.offset << std::endl;
            ostr << "buffer block index = " << bufferVar.blockIndex << std::endl;
            ostr << "array stride = " << bufferVar.arrayStride << std::endl;
            ostr << "matrix stride = " << bufferVar.matrixStride << std::endl;
            ostr << "is row major = " << bufferVar.isRowMajor << std::endl;
            ostr << "top level array size = " << bufferVar.topLevelArraySize << std::endl;
            ostr << "top level array stride = " << bufferVar.topLevelArrayStride << std::endl;
            auto const size = GetEnumSize(bufferVar.type, bufferVar.arraySize,
                bufferVar.arrayStride, bufferVar.matrixStride, bufferVar.isRowMajor);
            if (size > 0)
            {
                ostr << "size=" << size << std::endl;
            }
        }

        // Opaque type is not in a uniform block.
        else
        {
            ostr << "declaration = " << std::endl;
            ostr << "  buffer " << GetEnumShaderName(bufferVar.type).c_str() << " " << bufferVar.name.c_str() << ";" << std::endl;
        }

        ostr << std::endl;
    }
}

void GLSLReflection::ReflectProgramInputs()
{
    GLint numResources = 0;
    glGetProgramInterfaceiv(mHandle, GL_PROGRAM_INPUT, GL_ACTIVE_RESOURCES,
        &numResources);
    if (numResources > 0)
    {
        mInputs.resize(numResources);

        GLenum properties[] =
        {
            GL_NAME_LENGTH,
            GL_TYPE,
            GL_LOCATION,
            GL_ARRAY_SIZE,
            GL_REFERENCED_BY_VERTEX_SHADER,
            GL_REFERENCED_BY_GEOMETRY_SHADER,
            GL_REFERENCED_BY_FRAGMENT_SHADER,
            GL_REFERENCED_BY_COMPUTE_SHADER,
            GL_REFERENCED_BY_TESS_CONTROL_SHADER,
            GL_REFERENCED_BY_TESS_EVALUATION_SHADER,
            GL_IS_PER_PATCH
        };
        int const numProperties = sizeof(properties) / sizeof(int);
        GLint results[numProperties] = { 0 };
        for (int i = 0; i < numResources; ++i)
        {
            Input& info = mInputs[i];
            glGetProgramResourceiv(mHandle, GL_PROGRAM_INPUT, i,
                numProperties, properties, numProperties, nullptr, results);

            GLint numBytes = results[0] + 1;
            eastl::vector<GLchar> name(numBytes);
            glGetProgramResourceName(mHandle, GL_PROGRAM_INPUT, i, numBytes, nullptr, name.data());
            info.name = eastl::string(name.data());

            GLint* current = &results[1];
            info.type = *current++;
            info.location = *current++;
            info.arraySize = *current++;
            info.referencedBy[ST_VERTEX] = *current++;
            info.referencedBy[ST_GEOMETRY] = *current++;
            info.referencedBy[ST_PIXEL] = *current++;
            info.referencedBy[ST_COMPUTE] = *current++;
            info.referencedBy[ST_TESSCONTROL] = *current++;
            info.referencedBy[ST_TESSEVALUATION] = *current++;
            info.isPerPatch = *current++;
            info.locationComponent = *current++;
        }
    }
}

void GLSLReflection::ReflectProgramOutputs()
{
    GLint numResources = 0;
    glGetProgramInterfaceiv(mHandle, GL_PROGRAM_OUTPUT, GL_ACTIVE_RESOURCES,
        &numResources);
    if (numResources > 0)
    {
        mOutputs.resize(numResources);

        GLenum properties[] =
        {
            GL_NAME_LENGTH,
            GL_TYPE,
            GL_LOCATION,
            GL_ARRAY_SIZE,
            GL_REFERENCED_BY_VERTEX_SHADER,
            GL_REFERENCED_BY_GEOMETRY_SHADER,
            GL_REFERENCED_BY_FRAGMENT_SHADER,
            GL_REFERENCED_BY_COMPUTE_SHADER,
            GL_REFERENCED_BY_TESS_CONTROL_SHADER,
            GL_REFERENCED_BY_TESS_EVALUATION_SHADER,
            GL_IS_PER_PATCH,
            GL_LOCATION_INDEX
        };
        int const numProperties = sizeof(properties) / sizeof(int);
        GLint results[numProperties] = { 0 };
        for (int i = 0; i < numResources; ++i)
        {
            Output& info = mOutputs[i];
            glGetProgramResourceiv(mHandle, GL_PROGRAM_OUTPUT, i,
                numProperties, properties, numProperties, nullptr, results);

            GLint numBytes = results[0] + 1;
            eastl::vector<GLchar> name(numBytes);
            glGetProgramResourceName(mHandle, GL_PROGRAM_OUTPUT, i, numBytes, nullptr, name.data());
            info.name = eastl::string(name.data());

            GLint* current = &results[1];
            info.type = *current++;
            info.location = *current++;
            info.arraySize = *current++;
            info.referencedBy[ST_VERTEX] = *current++;
            info.referencedBy[ST_GEOMETRY] = *current++;
            info.referencedBy[ST_PIXEL] = *current++;
            info.referencedBy[ST_COMPUTE] = *current++;
            info.referencedBy[ST_TESSCONTROL] = *current++;
            info.referencedBy[ST_TESSEVALUATION] = *current++;
            info.isPerPatch = *current++;
            info.locationComponent = *current++;
            info.locationIndex = *current++;
        }
    }
}

void GLSLReflection::ReflectUniforms()
{
    GLint numResources = 0;
    glGetProgramInterfaceiv(mHandle, GL_UNIFORM, GL_ACTIVE_RESOURCES,
        &numResources);

    if (numResources > 0)
    {
        mUniforms.clear();

        GLenum properties[] =
        {
            GL_NAME_LENGTH,
            GL_TYPE,
            GL_LOCATION,
            GL_ARRAY_SIZE,
            GL_OFFSET,
            GL_BLOCK_INDEX,
            GL_ARRAY_STRIDE,
            GL_MATRIX_STRIDE,
            GL_IS_ROW_MAJOR,
            GL_ATOMIC_COUNTER_BUFFER_INDEX,
            GL_REFERENCED_BY_VERTEX_SHADER,
            GL_REFERENCED_BY_GEOMETRY_SHADER,
            GL_REFERENCED_BY_FRAGMENT_SHADER,
            GL_REFERENCED_BY_COMPUTE_SHADER,
            GL_REFERENCED_BY_TESS_CONTROL_SHADER,
            GL_REFERENCED_BY_TESS_EVALUATION_SHADER
        };
        int const numProperties = sizeof(properties) / sizeof(int);
        GLint results[numProperties] = { 0 };
        for (int i = 0; i < numResources; ++i)
        {
            Uniform info;
            glGetProgramResourceiv(mHandle, GL_UNIFORM, i,
                numProperties, properties, numProperties, nullptr, results);

            GLint numBytes = results[0] + 1;
            eastl::vector<GLchar> name(numBytes);
            glGetProgramResourceName(mHandle, GL_UNIFORM, i, numBytes, nullptr, name.data());
            info.name = eastl::string(name.data());

            GLint* current = &results[1];
            info.type = *current++;
            info.location = *current++;
            info.arraySize = *current++;
            info.offset = *current++;
            info.blockIndex = *current++;
            info.arrayStride = *current++;
            info.matrixStride = *current++;
            info.isRowMajor = *current++;
            info.atomicCounterBufferIndex = *current++;
            info.referencedBy[ST_VERTEX] = *current++;
            info.referencedBy[ST_GEOMETRY] = *current++;
            info.referencedBy[ST_PIXEL] = *current++;
            info.referencedBy[ST_COMPUTE] = *current++;
            info.referencedBy[ST_TESSCONTROL] = *current++;
            info.referencedBy[ST_TESSEVALUATION] = *current++;

            // To be sure the bufferBinding field is set correctly, use this approach.
            if (GL_INVALID_INDEX == static_cast<unsigned int>(info.blockIndex))
            {
                info.location = glGetUniformLocation(mHandle, info.name.c_str());
            }

            // Keep the original full name returned by the reflection.
            info.fullName = info.name;

            // For an array member, the name is of the form "someName[...]...".
            // The Engine Shader class currently wants "someName" because
            // that is how GLSL delivered the name.  Let's conform to that
            // for now.
            if (info.arraySize > 1)
            {
                auto index = info.name.find('[');
                if (index != eastl::string::npos)
                {
                    // Make sure array is not more than single dimensional.
                    auto index2 = info.name.find('[', index+1);
                    if (index2 != eastl::string::npos)
                    {
                        // TODO: Should this cause compile of shader to fail?
                        LogError("Only single dimensional arrays supported in GLSL uniforms.");
                    }

                    info.name = info.name.substr(0, index);
                }
                else
                {
                    // TODO: For now, trap any occurrence of an array member
                    // whose name is not of the form "someName[0]".
                    LogError("Unexpected condition.");
                }
            }

            mUniforms.push_back(info);
        }
    }
}

void GLSLReflection::ReflectDataBlocks(GLenum programInterface,
    eastl::vector<DataBlock>& blocks)
{
    GLint numResources = 0;
    glGetProgramInterfaceiv(mHandle, programInterface, GL_ACTIVE_RESOURCES,
        &numResources);

    if (numResources > 0)
    {
        blocks.resize(numResources);

        GLenum properties[] =
        {
            GL_NAME_LENGTH,
            GL_BUFFER_BINDING,
            GL_BUFFER_DATA_SIZE,
            GL_REFERENCED_BY_VERTEX_SHADER,
            GL_REFERENCED_BY_GEOMETRY_SHADER,
            GL_REFERENCED_BY_FRAGMENT_SHADER,
            GL_REFERENCED_BY_COMPUTE_SHADER,
            GL_REFERENCED_BY_TESS_CONTROL_SHADER,
            GL_REFERENCED_BY_TESS_EVALUATION_SHADER,
            GL_NUM_ACTIVE_VARIABLES
        };
        int const numProperties = sizeof(properties) / sizeof(int);
        GLint results[numProperties] = { 0 };
        for (int i = 0; i < numResources; ++i)
        {
            DataBlock& info = blocks[i];
            glGetProgramResourceiv(mHandle, programInterface, i,
                numProperties, properties, numProperties, nullptr, results);

            GLint numBytes = results[0] + 1;
            eastl::vector<GLchar> name(numBytes);
            glGetProgramResourceName(mHandle, programInterface, i, numBytes, nullptr, name.data());
            info.name = eastl::string(name.data());

            if (mVendorIsIntel && programInterface == GL_SHADER_STORAGE_BLOCK)
            {
                IntelWorkaround(info.name, results);
            }

            GLint* current = &results[1];
            info.bufferBinding = *current++;
            info.bufferDataSize = *current++;
            info.referencedBy[ST_VERTEX] = *current++;
            info.referencedBy[ST_GEOMETRY] = *current++;
            info.referencedBy[ST_PIXEL] = *current++;
            info.referencedBy[ST_COMPUTE] = *current++;
            info.referencedBy[ST_TESSCONTROL] = *current++;
            info.referencedBy[ST_TESSEVALUATION] = *current++;

            // To be sure the bufferBinding field is set correctly, use this approach.
            info.bufferBinding = glGetProgramResourceIndex(mHandle, programInterface, info.name.c_str());

            GLint numActiveVariables = *current++;
            if (numActiveVariables > 0)
            {
                info.activeVariables.resize(numActiveVariables);
                eastl::fill(info.activeVariables.begin(),
                    info.activeVariables.end(), 0);
                GLenum varProperty = GL_ACTIVE_VARIABLES;
                glGetProgramResourceiv(mHandle, programInterface, i, 1,
                    &varProperty, numActiveVariables, nullptr,
                    &info.activeVariables[0]);
            }
        }
    }
}

void GLSLReflection::ReflectAtomicCounterBuffers()
{
    GLint numResources = 0;
    glGetProgramInterfaceiv(mHandle, GL_ATOMIC_COUNTER_BUFFER,
        GL_ACTIVE_RESOURCES, &numResources);

    if (numResources > 0)
    {
        mAtomicCounterBuffers.resize(numResources);

        GLenum properties[] =
        {
            GL_BUFFER_BINDING,
            GL_BUFFER_DATA_SIZE,
            GL_REFERENCED_BY_VERTEX_SHADER,
            GL_REFERENCED_BY_GEOMETRY_SHADER,
            GL_REFERENCED_BY_FRAGMENT_SHADER,
            GL_REFERENCED_BY_COMPUTE_SHADER,
            GL_REFERENCED_BY_TESS_CONTROL_SHADER,
            GL_REFERENCED_BY_TESS_EVALUATION_SHADER,
            GL_NUM_ACTIVE_VARIABLES
        };
        int const numProperties = sizeof(properties) / sizeof(int);
        GLint results[numProperties] = { 0 };
        for (int i = 0; i < numResources; ++i)
        {
            AtomicCounterBuffer& info = mAtomicCounterBuffers[i];
            glGetProgramResourceiv(mHandle, GL_ATOMIC_COUNTER_BUFFER, i,
                numProperties, properties, numProperties, nullptr, results);

            GLint* current = &results[0];
            info.bufferBinding = *current++;
            info.bufferDataSize = *current++;
            info.referencedBy[ST_VERTEX] = *current++;
            info.referencedBy[ST_GEOMETRY] = *current++;
            info.referencedBy[ST_PIXEL] = *current++;
            info.referencedBy[ST_COMPUTE] = *current++;
            info.referencedBy[ST_TESSCONTROL] = *current++;
            info.referencedBy[ST_TESSEVALUATION] = *current++;

            GLint numActiveVariables = *current++;
            if (numActiveVariables > 0)
            {
                info.activeVariables.resize(numActiveVariables);
                eastl::fill(info.activeVariables.begin(),
                    info.activeVariables.end(), 0);
                GLenum varProperty = GL_ACTIVE_VARIABLES;
                glGetProgramResourceiv(mHandle, GL_ATOMIC_COUNTER_BUFFER, i,
                    1, &varProperty, numActiveVariables, nullptr,
                    &info.activeVariables[0]);
            }
        }
    }
}

void GLSLReflection::ReflectSubroutines(GLenum programInterface,
    eastl::vector<eastl::string>& subroutines)
{
    GLint numResources = 0;
    glGetProgramInterfaceiv(mHandle, programInterface, GL_ACTIVE_RESOURCES,
        &numResources);
    if (numResources > 0)
    {
        subroutines.resize(numResources);

        GLenum nameLengthProperty = GL_NAME_LENGTH;
        for (int i = 0; i < numResources; ++i)
        {
            GLint result = 0;
            glGetProgramResourceiv(mHandle, programInterface, i, 1,
                &nameLengthProperty, 1, nullptr, &result);

            GLint numBytes = result + 1;
            eastl::vector<GLchar> name(numBytes);
            glGetProgramResourceName(mHandle, programInterface, i, numBytes, nullptr, name.data());
            subroutines[i] = eastl::string(name.data());
        }
    }
}

void GLSLReflection::ReflectSubroutineUniforms(GLenum programInterface,
    eastl::vector<SubroutineUniform>& subUniforms)
{
    GLint numResources = 0;
    glGetProgramInterfaceiv(mHandle, programInterface, GL_ACTIVE_RESOURCES,
        &numResources);

    if (numResources > 0)
    {
        subUniforms.resize(numResources);

        GLenum properties[] =
        {
            GL_NAME_LENGTH,
            GL_LOCATION,
            GL_ARRAY_SIZE,
            GL_NUM_ACTIVE_VARIABLES
        };
        int const numProperties = sizeof(properties) / sizeof(int);
        GLint results[numProperties] = { 0 };
        for (int i = 0; i < numResources; ++i)
        {
            SubroutineUniform& info = subUniforms[i];
            glGetProgramResourceiv(mHandle, programInterface, i,
                numProperties, properties, numProperties, nullptr, results);

            GLint numBytes = results[0] + 1;
            eastl::vector<GLchar> name(numBytes);
            glGetProgramResourceName(mHandle, programInterface, i, numBytes, nullptr, name.data());
            info.name = eastl::string(name.data());

            GLint* current = &results[1];
            info.location = *current++;
            info.arraySize = *current++;

            GLint numCompatibleSubroutines = *current++;
            if (numCompatibleSubroutines > 0)
            {
                info.compatibleSubroutines.resize(numCompatibleSubroutines);
                eastl::fill(info.compatibleSubroutines.begin(),
                    info.compatibleSubroutines.end(), 0);
                GLenum subProperty = GL_COMPATIBLE_SUBROUTINES;
                glGetProgramResourceiv(mHandle, programInterface, i, 1,
                    &subProperty, numCompatibleSubroutines, nullptr,
                    &info.compatibleSubroutines[0]);
            }
        }
    }
}

void GLSLReflection::ReflectBufferVariables()
{
    GLint numResources = 0;
    glGetProgramInterfaceiv(mHandle, GL_BUFFER_VARIABLE, GL_ACTIVE_RESOURCES,
        &numResources);
    if (numResources > 0)
    {
        mBufferVariables.resize(numResources);

        GLenum properties[] =
        {
            GL_NAME_LENGTH,
            GL_TYPE,
            GL_ARRAY_SIZE,
            GL_OFFSET,
            GL_BLOCK_INDEX,
            GL_ARRAY_STRIDE,
            GL_MATRIX_STRIDE,
            GL_IS_ROW_MAJOR,
            GL_TOP_LEVEL_ARRAY_SIZE,
            GL_TOP_LEVEL_ARRAY_STRIDE,
            GL_REFERENCED_BY_VERTEX_SHADER,
            GL_REFERENCED_BY_GEOMETRY_SHADER,
            GL_REFERENCED_BY_FRAGMENT_SHADER,
            GL_REFERENCED_BY_COMPUTE_SHADER,
            GL_REFERENCED_BY_TESS_CONTROL_SHADER,
            GL_REFERENCED_BY_TESS_EVALUATION_SHADER
        };
        int const numProperties = sizeof(properties) / sizeof(int);
        GLint results[numProperties] = { 0 };
        for (int i = 0; i < numResources; ++i)
        {
            BufferVariable& info = mBufferVariables[i];
            glGetProgramResourceiv(mHandle, GL_BUFFER_VARIABLE, i,
                numProperties, properties, numProperties, nullptr, results);

            GLint numBytes = results[0] + 1;
            eastl::vector<GLchar> name(numBytes);
            glGetProgramResourceName(mHandle, GL_BUFFER_VARIABLE, i, numBytes, nullptr, name.data());
            info.name = eastl::string(name.data());

            GLint* current = &results[1];
            info.type = *current++;
            info.arraySize = *current++;
            info.offset = *current++;
            info.blockIndex = *current++;
            info.arrayStride = *current++;
            info.matrixStride = *current++;
            info.isRowMajor = *current++;
            info.topLevelArraySize = *current++;
            info.topLevelArrayStride = *current++;
            info.referencedBy[ST_VERTEX] = *current++;
            info.referencedBy[ST_GEOMETRY] = *current++;
            info.referencedBy[ST_PIXEL] = *current++;
            info.referencedBy[ST_COMPUTE] = *current++;
            info.referencedBy[ST_TESSCONTROL] = *current++;
            info.referencedBy[ST_TESSEVALUATION] = *current++;

            // Keep the original full name returned by the reflection.
            info.fullName = info.name;

            // Look up the buffer block name to which this variable belongs.
            auto const bufferBlockName = mShaderStorageBlocks[info.blockIndex].name + '.';

            // If the buffer variable begins with the name of the buffer block name,
            // then remove it.
            if (0 == info.name.find(bufferBlockName))
            {
                info.name = info.name.substr(bufferBlockName.length());
            }

            // For an array member, the name is of the form "someName[...]...".
            // The Engine Shader class currently wants "someName" because
            // that is how GLSL delivered the name.  Let's conform to that
            // for now.  But only if the [...] appears at the end of a name.
            if (info.topLevelArrayStride > 1)
            {
                // Drop anything before the last "." reference.
                auto index = info.name.find_last_of('.');
                if (index != eastl::string::npos)
                {
                    info.name = info.name.substr(index+1);
                }

                // Drop any [...] if it appears at end.
                index = info.name.find_last_of(']');
                if (index == info.name.length() - 1)
                {
                    index = info.name.find_last_of('[');
                    if (index != eastl::string::npos)
                    {
                        info.name = info.name.substr(0, index);
                    }
                }
            }
        }
    }
}

void GLSLReflection::ReflectTransformFeedbackVaryings()
{
    GLint numResources = 0;
    glGetProgramInterfaceiv(mHandle, GL_TRANSFORM_FEEDBACK_VARYING,
        GL_ACTIVE_RESOURCES, &numResources);
    if (numResources > 0)
    {
        mInputs.resize(numResources);

        GLenum properties[] =
        {
            GL_NAME_LENGTH,
            GL_TYPE,
            GL_ARRAY_SIZE,
            GL_OFFSET
        };
        int const numProperties = sizeof(properties) / sizeof(int);
        GLint results[numProperties] = { 0 };
        for (int i = 0; i < numResources; ++i)
        {
            TransformFeedbackVarying& info = mTransformFeedbackVaryings[i];
            glGetProgramResourceiv(mHandle, GL_TRANSFORM_FEEDBACK_VARYING, i,
                numProperties, properties, numProperties, nullptr, results);

            GLint numBytes = results[0] + 1;
            eastl::vector<GLchar> name(numBytes);
            glGetProgramResourceName(mHandle, GL_TRANSFORM_FEEDBACK_VARYING, i, numBytes, nullptr, name.data());
            info.name = eastl::string(name.data());

            GLint* current = &results[1];
            info.type = *current++;
            info.arraySize = *current++;
            info.offset = *current++;
            info.transformFeedbackBufferIndex = *current++;
        }
    }
}

void GLSLReflection::ReflectTransformFeedbackBuffers()
{
    GLint numResources = 0;
    glGetProgramInterfaceiv(mHandle, GL_TRANSFORM_FEEDBACK_BUFFER,
        GL_ACTIVE_RESOURCES, &numResources);
    if (numResources > 0)
    {
        mTransformFeedbackBuffers.resize(numResources);

        GLenum properties[] =
        {
            GL_BUFFER_BINDING,
            GL_NUM_ACTIVE_VARIABLES
        };
        int const numProperties = sizeof(properties) / sizeof(int);
        GLint results[numProperties] = { 0 };
        for (int i = 0; i < numResources; ++i)
        {
            TransformFeedbackBuffer& info = mTransformFeedbackBuffers[i];
            glGetProgramResourceiv(mHandle, GL_TRANSFORM_FEEDBACK_BUFFER, i,
                numProperties, properties, numProperties, nullptr, results);

            GLint* current = &results[0];
            info.bufferBinding = *current++;
            info.transformFeedbackBufferStride = *current++;

            GLint numActiveVariables = *current++;
            if (numActiveVariables > 0)
            {
                info.activeVariables.resize(numActiveVariables);
                eastl::fill(info.activeVariables.begin(),
                    info.activeVariables.end(), 0);
                GLenum varProperty = GL_ACTIVE_VARIABLES;
                glGetProgramResourceiv(mHandle, GL_TRANSFORM_FEEDBACK_BUFFER,
                    i, 1, &varProperty, numActiveVariables, nullptr,
                    &info.activeVariables[0]);
            }
        }
    }
}

unsigned GLSLReflection::GetEnumSize(GLenum value, GLint arraySize, GLint arrayStride, GLint matrixStride, GLint isRowMajor)
{
    for (int i = 0; 0 != msEnumMap[i].value; ++i)
    {
        auto const& item = msEnumMap[i];
        if (item.value == value)
        {
            if (arrayStride > 0)
            {
                return arrayStride * arraySize;
            }
            else if (matrixStride > 0)
            {
                if (isRowMajor)
                {
                    return item.rows * matrixStride;
                }
                else
                {
                    return item.cols * matrixStride;
                }
            }
            else if (item.rows > 0)
            {
                return item.rows * item.size;
            }
            return 0;
        }
    }

    return 0;
}

eastl::string GLSLReflection::GetEnumName(GLenum value)
{
    for (int i = 0; 0 != msEnumMap[i].value; ++i)
    {
        auto const& item = msEnumMap[i];
        if (item.value == value)
        {
            return item.name;
        }
    }

    return eastl::string("unknown(type=") + eastl::to_string(value) + ")";
}

eastl::string GLSLReflection::GetEnumShaderName(GLenum value)
{
    for (int i = 0; 0 != msEnumMap[i].value; ++i)
    {
        auto const& item = msEnumMap[i];
        if (item.value == value)
        {
            return item.shaderName;
        }
    }

    return eastl::string("unknown(type=") + eastl::to_string(value) + ")";
}

eastl::string GLSLReflection::GetReferencedByShaderList(GLint const referencedBy[6])
{
    eastl::string strList;

    if (referencedBy[ST_VERTEX]) strList += "vertex ";
    if (referencedBy[ST_GEOMETRY]) strList += "geometry ";
    if (referencedBy[ST_PIXEL]) strList += "pixel ";
    if (referencedBy[ST_COMPUTE]) strList += "compute ";
    if (referencedBy[ST_TESSCONTROL]) strList += "tessControl ";
    if (referencedBy[ST_TESSEVALUATION]) strList += "tessEvaluation ";

    return strList;
}

void GLSLReflection::IntelWorkaround(eastl::string const& name, GLint results[])
{
    // For each shader reported as NOT referencing the buffer 'name', search
    // the shader source code for 'buffer name { Type data[]; } instance'.
    // If found, then search the remaining strings for 'instance'.  If a match
    // is found, then change the results[] reference value from 0 to 1.
    GLsizei maxCount = 0;
    glGetProgramiv(mHandle, GL_ATTACHED_SHADERS, &maxCount);
    if (maxCount < 0)
    {
        return;
    }

    eastl::vector<GLuint> shaders(maxCount);
    GLsizei count = 0;
    glGetAttachedShaders(mHandle, maxCount, &count, shaders.data());
    if (count != maxCount)
    {
        return;
    }

    for (auto shader : shaders)
    {
        GLint type = 0;
        glGetShaderiv(shader, GL_SHADER_TYPE, &type);
        auto iter = mShaderTypeMap.find(type);
        if (iter != mShaderTypeMap.end())
        {
            int index = iter->second;
            if (results[index] == 0)
            {
                // The shader is reported as not referenced.  Verify
                // whether or not this is correct.
                GLint length = 0;
                glGetShaderiv(shader, GL_SHADER_SOURCE_LENGTH, &length);
                if (length <= 0)
                {
                    return;
                }

                eastl::vector<GLchar> rawSource(length);
                glGetShaderSource(shader, length, nullptr, rawSource.data());
                eastl::string source(rawSource.data());

                // Find 'buffer name { Type member[]; } instance;'.
                auto beginInstance = source.find("buffer " + name);
                if (beginInstance == eastl::string::npos)
                {
                    return;
                }

                beginInstance = source.find('}', beginInstance);
                if (beginInstance == eastl::string::npos)
                {
                    return;
                }

                if (++beginInstance >= source.length())
                {
                    return;
                }

                if (source[beginInstance] == ';')
                {
                    // Found 'buffer name { Type member[]; };
                    // TODO: The shader can reference the buffer only via the
                    // 'member' name.  We still need to determine whether this
                    // happens.  For now in Engine samples, we always use an
                    // 'instance'.
                    return;
                }

                beginInstance = source.find_first_not_of(" \t", beginInstance);
                if (beginInstance == eastl::string::npos)
                {
                    return;
                }

                auto endInstance = source.find_first_of(" ;\t", beginInstance);
                eastl::string instance = source.substr(beginInstance, endInstance - beginInstance);

                // We have found the 'instance' of the buffer.  If it is referenced
                // later in the shader, convert the reference result to 1.
                if (source.find(instance, endInstance) != eastl::string::npos)
                {
                    results[index] = 1;
                }
            }
        }
    }
}

#define ENUM(value, shadername, rows, cols, size) { value, #value, #shadername, rows, cols, size }
const GLSLReflection::EnumMap GLSLReflection::msEnumMap[]
{
    ENUM(GL_FLOAT,  float  , 1, 0, 4),
    ENUM(GL_FLOAT_VEC2,  vec2  , 2, 0, 4),
    ENUM(GL_FLOAT_VEC3,  vec3  , 3, 0, 4),
    ENUM(GL_FLOAT_VEC4,  vec4  , 4, 0, 4),
    ENUM(GL_DOUBLE,  double  , 1, 0, 8),
    ENUM(GL_DOUBLE_VEC2,  dvec2  , 2, 0, 8),
    ENUM(GL_DOUBLE_VEC3,  dvec3  , 3, 0, 8),
    ENUM(GL_DOUBLE_VEC4,  dvec4  , 4, 0, 8),
    ENUM(GL_INT,  int  , 1, 0, 4),
    ENUM(GL_INT_VEC2,  ivec2  , 2, 0, 4),
    ENUM(GL_INT_VEC3,  ivec3  , 3, 0, 4),
    ENUM(GL_INT_VEC4,  ivec4  , 4, 0, 4),
    ENUM(GL_UNSIGNED_INT,  uint  , 1, 0, 4),
    ENUM(GL_UNSIGNED_INT_VEC2,  uvec2  , 2, 0, 4),
    ENUM(GL_UNSIGNED_INT_VEC3,  uvec3  , 3, 0, 4),
    ENUM(GL_UNSIGNED_INT_VEC4,  uvec4  , 4, 0, 4),
    ENUM(GL_BOOL,  bool  , 1, 0, 4),
    ENUM(GL_BOOL_VEC2,  bvec2  , 2, 0, 4),
    ENUM(GL_BOOL_VEC3,  bvec3  , 3, 0, 4),
    ENUM(GL_BOOL_VEC4,  bvec4  , 4, 0, 4),
    ENUM(GL_FLOAT_MAT2,  mat2  , 2, 2, 4),
    ENUM(GL_FLOAT_MAT3,  mat3  , 3, 3, 4),
    ENUM(GL_FLOAT_MAT4,  mat4  , 4, 4, 4),
    ENUM(GL_FLOAT_MAT2x3,  mat2x3  , 2, 3, 4),
    ENUM(GL_FLOAT_MAT2x4,  mat2x4  , 2, 4, 4),
    ENUM(GL_FLOAT_MAT3x2,  mat3x2  , 3, 2, 4),
    ENUM(GL_FLOAT_MAT3x4,  mat3x4  , 3, 4, 4),
    ENUM(GL_FLOAT_MAT4x2,  mat4x2  , 4, 2, 4),
    ENUM(GL_FLOAT_MAT4x3,  mat4x3  , 4, 3, 4),
    ENUM(GL_DOUBLE_MAT2,  dmat2  , 2, 2, 8),
    ENUM(GL_DOUBLE_MAT3,  dmat3  , 3, 3, 8),
    ENUM(GL_DOUBLE_MAT4,  dmat4  , 4, 4, 8),
    ENUM(GL_DOUBLE_MAT2x3,  dmat2x3  , 2, 3, 8),
    ENUM(GL_DOUBLE_MAT2x4,  dmat2x4  , 2, 4, 8),
    ENUM(GL_DOUBLE_MAT3x2,  dmat3x2  , 3, 2, 8),
    ENUM(GL_DOUBLE_MAT3x4,  dmat3x4  , 3, 4, 8),
    ENUM(GL_DOUBLE_MAT4x2,  dmat4x2  , 4, 2, 8),
    ENUM(GL_DOUBLE_MAT4x3,  dmat4x3  , 4, 3, 8),
    ENUM(GL_SAMPLER_1D,  sampler1D  , 0, 0, 0),
    ENUM(GL_SAMPLER_2D,  sampler2D  , 0, 0, 0),
    ENUM(GL_SAMPLER_3D,  sampler3D  , 0, 0, 0),
    ENUM(GL_SAMPLER_CUBE,  samplerCube  , 0, 0, 0),
    ENUM(GL_SAMPLER_1D_SHADOW,  sampler1DShadow  , 0, 0, 0),
    ENUM(GL_SAMPLER_2D_SHADOW,  sampler2DShadow  , 0, 0, 0),
    ENUM(GL_SAMPLER_1D_ARRAY,  sampler1DArray  , 0, 0, 0),
    ENUM(GL_SAMPLER_2D_ARRAY,  sampler2DArray  , 0, 0, 0),
    ENUM(GL_SAMPLER_1D_ARRAY_SHADOW,  sampler1DArrayShadow  , 0, 0, 0),
    ENUM(GL_SAMPLER_2D_ARRAY_SHADOW,  sampler2DArrayShadow  , 0, 0, 0),
    ENUM(GL_SAMPLER_2D_MULTISAMPLE,  sampler2DMS  , 0, 0, 0),
    ENUM(GL_SAMPLER_2D_MULTISAMPLE_ARRAY,  sampler2DMSArray  , 0, 0, 0),
    ENUM(GL_SAMPLER_CUBE_SHADOW,  samplerCubeShadow  , 0, 0, 0),
    ENUM(GL_SAMPLER_CUBE_MAP_ARRAY, samplerCubeArray, 0, 0, 0),
    ENUM(GL_SAMPLER_CUBE_MAP_ARRAY_SHADOW, samplerCubeArrayShadow, 0, 0, 0),
    ENUM(GL_SAMPLER_BUFFER,  samplerBuffer  , 0, 0, 0),
    ENUM(GL_SAMPLER_2D_RECT,  sampler2DRect  , 0, 0, 0),
    ENUM(GL_SAMPLER_2D_RECT_SHADOW,  sampler2DRectShadow  , 0, 0, 0),
    ENUM(GL_INT_SAMPLER_1D,  isampler1D  , 0, 0, 0),
    ENUM(GL_INT_SAMPLER_2D,  isampler2D  , 0, 0, 0),
    ENUM(GL_INT_SAMPLER_3D,  isampler3D  , 0, 0, 0),
    ENUM(GL_INT_SAMPLER_CUBE,  isamplerCube  , 0, 0, 0),
    ENUM(GL_INT_SAMPLER_1D_ARRAY,  isampler1DArray  , 0, 0, 0),
    ENUM(GL_INT_SAMPLER_2D_ARRAY,  isampler2DArray  , 0, 0, 0),
    ENUM(GL_INT_SAMPLER_2D_MULTISAMPLE,  isampler2DMS  , 0, 0, 0),
    ENUM(GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY,  isampler2DMSArray  , 0, 0, 0),
    ENUM(GL_INT_SAMPLER_CUBE_MAP_ARRAY,  isamplerCubeArray  , 0, 0, 0),
    ENUM(GL_INT_SAMPLER_BUFFER,  isamplerBuffer  , 0, 0, 0),
    ENUM(GL_INT_SAMPLER_2D_RECT,  isampler2DRect  , 0, 0, 0),
    ENUM(GL_UNSIGNED_INT_SAMPLER_1D,  usampler1D  , 0, 0, 0),
    ENUM(GL_UNSIGNED_INT_SAMPLER_2D,  usampler2D  , 0, 0, 0),
    ENUM(GL_UNSIGNED_INT_SAMPLER_3D,  usampler3D  , 0, 0, 0),
    ENUM(GL_UNSIGNED_INT_SAMPLER_CUBE,  usamplerCube  , 0, 0, 0),
    ENUM(GL_UNSIGNED_INT_SAMPLER_1D_ARRAY,  usampler2DArray  , 0, 0, 0),
    ENUM(GL_UNSIGNED_INT_SAMPLER_2D_ARRAY,  usampler2DArray  , 0, 0, 0),
    ENUM(GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE,  usampler2DMS  , 0, 0, 0),
    ENUM(GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY,  usampler2DMSArray  , 0, 0, 0),
    ENUM(GL_UNSIGNED_INT_SAMPLER_CUBE_MAP_ARRAY,  usamplerCubeArray  , 0, 0, 0),
    ENUM(GL_UNSIGNED_INT_SAMPLER_BUFFER,  usamplerBuffer  , 0, 0, 0),
    ENUM(GL_UNSIGNED_INT_SAMPLER_2D_RECT,  usampler2DRect  , 0, 0, 0),
    ENUM(GL_IMAGE_1D,  image1D  , 0, 0, 0),
    ENUM(GL_IMAGE_2D,  image2D  , 0, 0, 0),
    ENUM(GL_IMAGE_3D,  image3D  , 0, 0, 0),
    ENUM(GL_IMAGE_CUBE,  imageCube  , 0, 0, 0),
    ENUM(GL_IMAGE_1D_ARRAY,  image1DArray  , 0, 0, 0),
    ENUM(GL_IMAGE_2D_ARRAY,  image2DArray  , 0, 0, 0),
    ENUM(GL_IMAGE_2D_MULTISAMPLE,  image2DMS  , 0, 0, 0),
    ENUM(GL_IMAGE_2D_MULTISAMPLE_ARRAY,  image2DMSArray  , 0, 0, 0),
    ENUM(GL_IMAGE_CUBE_MAP_ARRAY,  imageCubeArray  , 0, 0, 0),
    ENUM(GL_IMAGE_BUFFER,  imageBuffer  , 0, 0, 0),
    ENUM(GL_IMAGE_2D_RECT,  image2DRect  , 0, 0, 0),
    ENUM(GL_INT_IMAGE_1D,  iimage1D  , 0, 0, 0),
    ENUM(GL_INT_IMAGE_2D,  iimage2D  , 0, 0, 0),
    ENUM(GL_INT_IMAGE_3D,  iimage3D  , 0, 0, 0),
    ENUM(GL_INT_IMAGE_CUBE,  iimageCube  , 0, 0, 0),
    ENUM(GL_INT_IMAGE_1D_ARRAY,  iimage1DArray  , 0, 0, 0),
    ENUM(GL_INT_IMAGE_2D_ARRAY,  iimage2DArray  , 0, 0, 0),
    ENUM(GL_INT_IMAGE_2D_MULTISAMPLE,  iimage2DMS  , 0, 0, 0),
    ENUM(GL_INT_IMAGE_2D_MULTISAMPLE_ARRAY,  iimage2DMSArray  , 0, 0, 0),
    ENUM(GL_INT_IMAGE_CUBE_MAP_ARRAY,  iimageCubeArray  , 0, 0, 0),
    ENUM(GL_INT_IMAGE_BUFFER,  iimageBuffer  , 0, 0, 0),
    ENUM(GL_INT_IMAGE_2D_RECT,  iimage2DRect  , 0, 0, 0),
    ENUM(GL_UNSIGNED_INT_IMAGE_1D,  uimage1D  , 0, 0, 0),
    ENUM(GL_UNSIGNED_INT_IMAGE_2D,  uimage2D  , 0, 0, 0),
    ENUM(GL_UNSIGNED_INT_IMAGE_3D,  uimage3D  , 0, 0, 0),
    ENUM(GL_UNSIGNED_INT_IMAGE_CUBE,  uimageCube  , 0, 0, 0),
    ENUM(GL_UNSIGNED_INT_IMAGE_1D_ARRAY,  uimage2DArray  , 0, 0, 0),
    ENUM(GL_UNSIGNED_INT_IMAGE_2D_ARRAY,  uimage2DArray  , 0, 0, 0),
    ENUM(GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE,  uimage2DMS  , 0, 0, 0),
    ENUM(GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY,  uimage2DMSArray  , 0, 0, 0),
    ENUM(GL_UNSIGNED_INT_IMAGE_CUBE_MAP_ARRAY, uimageCubeArray, 0, 0, 0),
    ENUM(GL_UNSIGNED_INT_IMAGE_BUFFER,  uimageBuffer  , 0, 0, 0),
    ENUM(GL_UNSIGNED_INT_IMAGE_2D_RECT,  uimage2DRect  , 0, 0, 0),
    ENUM(GL_UNSIGNED_INT_ATOMIC_COUNTER, atomic_uint  , 0, 0, 0),
    {0, "", "", 0, 0, 0 }
};
#undef ENUM
