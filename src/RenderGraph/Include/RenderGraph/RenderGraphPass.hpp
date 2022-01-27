#ifndef RG_RENDERGRAPHPASS_HPP
#define RG_RENDERGRAPHPASS_HPP

#include "RenderGraph/RenderGraphExport.hpp"

#include <vector>

namespace RG {

class Operation;
class Resource;

class RENDERGRAPH_DLL_EXPORT Pass {
public:
    struct OperationIO {
        Operation*          op;
        std::vector<Resource*> inputs;
        std::vector<Resource*> outputs;
    };

    struct ResourceIO {
        Resource*            res;
        std::vector<Operation*> writers;
        std::vector<Operation*> readers;
    };

private:
    std::vector<OperationIO> operationIOs;
    std::vector<ResourceIO>  resourceIOs;

public:
    std::vector<Operation*> GetAllOperations () const;
    std::vector<Resource*>  GetAllInputs () const;
    std::vector<Resource*>  GetAllOutputs () const;

    void AddOutput (Operation* op, Resource* output);
    void AddInput (Operation* op, Resource* input);

    bool RemoveOutput (Operation* op, Resource* output);
    bool RemoveInput (Operation* op, Resource* input);

    ResourceIO*  GetResourceIO (Resource* res);
    OperationIO* GetOperationIO (Operation* op);

    bool RemoveOperationIO (OperationIO*);
    bool AddOperationIO (OperationIO*);

    bool IsEmpty () const;

private:
    bool RemoveIfEmpty (Operation* op);
    bool RemoveIfEmpty (Resource* res);

    ResourceIO* EnsureResourceIO (Resource* res);
    OperationIO* EnsureOperationIO (Operation* op);
};

} // namespace RG

#endif