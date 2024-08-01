#pragma once
#include "GLUtils.hpp"

template <typename SurfT>
[[nodiscard]] MeshObject<Vertex> GetParamSurfMesh( const SurfT& surf, const std::size_t N = 80, const std::size_t M = 40 )
{
    MeshObject<Vertex> outputMesh;

	// We approximate our parametric surface with NxM rectangles => must be evaluated at (N+1)x(M+1) points
	outputMesh.vertexArray.resize((N + 1) * (M + 1));


	for (std::size_t j = 0; j <= M; ++j)
	{
		for (std::size_t i = 0; i <= N; ++i)
		{
			float u = i / (float)N;
			float v = j / (float)M;

			std::size_t index = i + j * (N + 1);
			outputMesh.vertexArray[index].position = surf.GetPos(u, v);
			outputMesh.vertexArray[index].normal   = surf.GetNorm(u, v);
			outputMesh.vertexArray[index].texcoord = surf.GetTex(u, v);
		}
	}



	// Index buffer data: NxM rectangle = 2xNxM triangle = 3x2xNxM index for triangle list
	outputMesh.indexArray.resize(3 * 2 * (N) * (M));

	for (std::size_t j = 0; j < M; ++j)
	{
		for (std::size_t i = 0; i < N; ++i)
		{
			// For each rectangle, we make two triangles, 
			// which are connecting the points corresponding to the 
			// parameter values (u_i, v_j) generated at the following (i,j) indices:
			// 
			// (i,j+1) C-----D (i+1,j+1)
			//         |\    |				A = p(u_i, v_j)
			//         | \   |				B = p(u_{i+1}, v_j)
			//         |  \  |				C = p(u_i, v_{j+1})
			//         |   \ |				D = p(u_{i+1}, v_{j+1})
			//         |    \|
			//   (i,j) A-----B (i+1,j)
			//
			// - the 1D index for (i,j) in the VBO: i+j*(N+1)
			// - 1D index for (i,j) in IB: i*6+j*6*N
			//		(because every rectangle has 2 triangles = 6 indices)
			//
			std::size_t index = i * 6 + j * (6 * N);
			outputMesh.indexArray[ index + 0 ] = static_cast<GLuint>( ( i     ) + ( j     ) * ( N + 1 ) );
			outputMesh.indexArray[ index + 1 ] = static_cast<GLuint>( ( i     ) + ( j + 1 ) * ( N + 1 ) );
			outputMesh.indexArray[ index + 2 ] = static_cast<GLuint>( ( i + 1 ) + ( j     ) * ( N + 1 ) );
			outputMesh.indexArray[ index + 3 ] = static_cast<GLuint>( ( i + 1 ) + ( j     ) * ( N + 1 ) );
			outputMesh.indexArray[ index + 4 ] = static_cast<GLuint>( ( i     ) + ( j + 1 ) * ( N + 1 ) );
			outputMesh.indexArray[ index + 5 ] = static_cast<GLuint>( ( i + 1 ) + ( j + 1 ) * ( N + 1 ) );
		}
	}
    
        return outputMesh;
}