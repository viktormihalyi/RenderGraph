#include "RenderGraphPass.hpp"
#include "Operation.hpp"
#include "Resource.hpp"
#include "Drawable.hpp"
#include "ShaderPipeline.hpp"
#include "ComputeShaderPipeline.hpp"

#include "Utils/Assert.hpp"

#include "VulkanWrapper/Framebuffer.hpp"
#include "VulkanWrapper/GraphicsPipeline.hpp"
#include "VulkanWrapper/ComputePipeline.hpp"
#include "VulkanWrapper/PipelineLayout.hpp"
#include "VulkanWrapper/RenderPass.hpp"
#include "VulkanWrapper/ShaderModule.hpp"
#include "VulkanWrapper/PipelineLayout.hpp"
#include "VulkanWrapper/DescriptorSet.hpp"
#include "VulkanWrapper/DescriptorSetLayout.hpp"

#include <optional>
#include <set>


namespace RG {

std::vector<Operation*> Pass::GetAllOperations () const
{
    std::vector<Operation*> result;
    std::set<Operation*> resultSet;
    for (auto& op : operationIOs) {
        if (resultSet.insert (op.op).second) {
            result.push_back (op.op);
        }
    }
    return result;
}


std::vector<Resource*> Pass::GetAllInputs () const
{
    std::set<Resource*> resultSet;
    std::vector<Resource*> result;
    for (auto& op : operationIOs) {
        for (Resource* inp : op.inputs) {
            if (resultSet.insert (inp).second) {
                result.push_back (inp);
            }
        }
    }
    return result;
}


std::vector<Resource*> Pass::GetAllOutputs () const
{
    std::vector<Resource*> result;
    std::set<Resource*> resultSet;
    for (auto& op : operationIOs) {
        for (Resource* out : op.outputs) {
            if (resultSet.insert (out).second) {
                result.push_back (out);
            }
        }
    }
    return result;
}


template <typename T>
static bool Contains (const std::vector<T>& vec, const T& value)
{
    return std::find (vec.begin (), vec.end (), value) != vec.end ();
}


template<typename T>
static void InsertIfNotContains (std::vector<T>& vec, const T& value)
{
    if (!Contains (vec, value))
        vec.push_back (value);
}


template<typename Container, typename Type>
static void Remove (Container& vec, Type& value)
{
    vec.erase (std::remove (vec.begin (), vec.end (), value), vec.end ());
}


void Pass::AddOutput (Operation* op, Resource* output)
{
    OperationIO* oIO = EnsureOperationIO (op);
    ResourceIO*  rIO = EnsureResourceIO (output);

    InsertIfNotContains (oIO->outputs, output);
    InsertIfNotContains (rIO->writers, op);
}


void Pass::AddInput (Operation* op, Resource* input)
{
    OperationIO* oIO = EnsureOperationIO (op);
    ResourceIO*  rIO = EnsureResourceIO (input);

    InsertIfNotContains (oIO->inputs, input);
    InsertIfNotContains (rIO->readers, op);
}


bool Pass::RemoveOutput (Operation* op, Resource* output)
{
    OperationIO* oIO = GetOperationIO (op);
    ResourceIO*  rIO = GetResourceIO (output);

    if (GVK_ERROR (oIO == nullptr || rIO == nullptr)) {
        return false;
    }

    if (GVK_ERROR (!Contains (oIO->outputs, output) || !Contains (rIO->writers, op))) {
        return false;
    }

    Remove (oIO->outputs, output);
    Remove (rIO->writers, op);

    RemoveIfEmpty (op);
    RemoveIfEmpty (output);

    return true;
}


bool Pass::RemoveInput (Operation* op, Resource* input)
{
    OperationIO* oIO = GetOperationIO (op);
    ResourceIO*  rIO = GetResourceIO (input);

    if (GVK_ERROR (oIO == nullptr || rIO == nullptr)) {
        return false;
    }

    if (GVK_ERROR (!Contains (oIO->inputs, input) || !Contains (rIO->readers, op))) {
        return false;
    }

    Remove (oIO->inputs, input);
    Remove (rIO->readers, op);

    RemoveIfEmpty (op);
    RemoveIfEmpty (input);

    return true;
}


Pass::ResourceIO* Pass::GetResourceIO (Resource* res)
{
    for (ResourceIO& rIO : resourceIOs) {
        if (rIO.res == res) {
            return &rIO;
        }
    }
    return nullptr;
}


Pass::OperationIO* Pass::GetOperationIO (Operation* op)
{
    for (OperationIO& oIO : operationIOs) {
        if (oIO.op == op) {
            return &oIO;
        }
    }
    return nullptr;
}


bool Pass::RemoveOperationIO (OperationIO* op)
{
    bool found = false;

    for (size_t i = 0; i < operationIOs.size (); ++i) {
        if (&operationIOs[i] == op) {
            found = true;
        }
    }

    if (GVK_ERROR (!found))
        return false;

    const std::vector<Resource*> opOutputs = op->outputs;
    const std::vector<Resource*> opInputs  = op->inputs;

    for (Resource* output : opOutputs)
        RemoveOutput (op->op, output);

    for (Resource* input : opInputs)
        RemoveInput (op->op, input);

    return true;
}


bool Pass::AddOperationIO (OperationIO* op)
{
    bool found = false;

    for (size_t i = 0; i < operationIOs.size (); ++i) {
        if (&operationIOs[i] == op) {
            found = true;
        }
    }

    if (GVK_ERROR (found))
        return false;

    for (Resource* output : op->outputs)
        AddOutput (op->op, output);

    for (Resource* input : op->inputs)
        AddInput (op->op, input);

    return true;
}



bool Pass::IsEmpty () const
{
    return operationIOs.empty () && resourceIOs.empty ();
}


bool Pass::RemoveIfEmpty (Operation* op)
{
    for (size_t i = 0; i < operationIOs.size (); ++i) {
        if (operationIOs[i].op == op && operationIOs[i].inputs.empty () && operationIOs[i].outputs.empty ()) {
            operationIOs.erase (operationIOs.begin () + i);
            return true;
        }
    }

    return false;
}


bool Pass::RemoveIfEmpty (Resource* res)
{
    for (size_t i = 0; i < resourceIOs.size (); ++i) {
        if (resourceIOs[i].res == res && resourceIOs[i].writers.empty () && resourceIOs[i].readers.empty ()) {
            resourceIOs.erase (resourceIOs.begin () + i);
            return true;
        }
    }

    return false;
}


Pass::ResourceIO* Pass::EnsureResourceIO (Resource* res)
{
    ResourceIO* existing = GetResourceIO (res);
    if (existing != nullptr) {
        return existing;
    }

    resourceIOs.emplace_back (ResourceIO { res, {}, {} });
    return &resourceIOs.back ();
}


Pass::OperationIO* Pass::EnsureOperationIO (Operation* op)
{
    OperationIO* existing = GetOperationIO (op);
    if (existing != nullptr) {
        return existing;
    }

    operationIOs.emplace_back (OperationIO { op, {}, {} });
    return &operationIOs.back ();
}


} // namespace RG
