#ifndef RENDERGRAPH_RENDERGRAPHEXPORT_HPP
#define RENDERGRAPH_RENDERGRAPHEXPORT_HPP

#ifdef _WIN32
#ifdef RenderGraph_EXPORTS
#define RENDERGRAPH_DLL_EXPORT __declspec(dllexport)
#else
#define RENDERGRAPH_DLL_EXPORT __declspec(dllimport)
#endif
#else
#ifdef RenderGraph_EXPORTS
#define RENDERGRAPH_DLL_EXPORT __attribute__ ((__visibility__ ("default")))
#else
#define RENDERGRAPH_DLL_EXPORT
#endif
#endif


#endif