#ifndef RENDERGRAPH_UNIFORM_REFLECTION_HPP
#define RENDERGRAPH_UNIFORM_REFLECTION_HPP

#include "RenderGraph/RenderGraphExport.hpp"
#include "RenderGraph/BufferView.hpp"
#include "RenderGraph/Operation.hpp"
#include "RenderGraph/Resource.hpp"

#include "RenderGraph/VulkanWrapper/ShaderModule.hpp"
#include "RenderGraph/VulkanWrapper/ShaderReflection.hpp"

#include "RenderGraph/Utils/Assert.hpp"
#include "RenderGraph/Utils/UUID.hpp"

#include <glm/glm.hpp>

#include <tuple>
#include <unordered_map>
#include <map>
#include <vector>

namespace RG {

// TODO add possibility to store uniforms in gpu memory

// adds resources to and existing rendergraph based on the renderoperations inside it
// modifying uniforms takes place in a staging cpu memory, calling Flush will copy these to the actual uniform memory
// accessing a uniforms is available with operator[] eg.: reflection[std::shared_ptr<RG::Operation>][ShaderKind][std::string][std::string]...

class RENDERGRAPH_DLL_EXPORT ImageMap {
private:
    std::vector<std::pair<SR::Sampler, std::shared_ptr<ReadOnlyImageResource>>> images;

public:
    ImageMap ();

    std::shared_ptr<ReadOnlyImageResource> FindByName (const std::string& name) const;

    void Put (const SR::Sampler& sampler, const std::shared_ptr<ReadOnlyImageResource>& res);
};

using CreateParams                 = std::tuple<glm::uvec3, VkFormat, VkFilter>;
using ExtentProviderForImageCreate = std::function<std::optional<CreateParams> (const SR::Sampler& sampler)>;


RENDERGRAPH_DLL_EXPORT
ImageMap CreateEmptyImageResources (RG::ConnectionSet& connectionSet);

RENDERGRAPH_DLL_EXPORT
ImageMap CreateEmptyImageResources (RG::ConnectionSet& connectionSet, const ExtentProviderForImageCreate& extentProvider);


class RENDERGRAPH_DLL_EXPORT UniformReflection {
private:
    class RENDERGRAPH_DLL_EXPORT BufferObjectSelector {
    private:
        std::map<std::string, std::shared_ptr<SR::IBufferData>, std::less<>> udatas;

    public:
        SR::IBufferData& operator[] (std::string_view bufferObjectName);
        
        void Set (const std::string& bufferObjectName, const std::shared_ptr<SR::IBufferData>& bufferObjectData);
        
        bool Contains (std::string_view bufferObjectName) const;

        friend class UniformReflection;
    };

    class RENDERGRAPH_DLL_EXPORT ShaderKindSelector {
    private:
        std::unordered_map<GVK::ShaderKind, BufferObjectSelector> bufferObjectSelectors;

    public:
        BufferObjectSelector& operator[] (GVK::ShaderKind shaderKind);

        void Set (GVK::ShaderKind shaderKind, BufferObjectSelector&& bufferObjectSelector);

        friend class UniformReflection;
    };

    std::unordered_map<GVK::UUID, ShaderKindSelector> selectors;

public:

    using ResourceCreator = std::function<std::shared_ptr<RG::DescriptorBindableBufferResource> (const std::shared_ptr<RG::Operation>&, const GVK::ShaderModule&, const std::shared_ptr<SR::BufferObject>&, bool& treatAsOutput)>;

    static std::shared_ptr<RG::DescriptorBindableBufferResource> DefaultResourceCreator (const std::shared_ptr<RG::Operation>&, const GVK::ShaderModule&, const std::shared_ptr<SR::BufferObject>& bufferObject, bool&)
    {
        return std::make_unique<RG::CPUBufferResource> (bufferObject->GetFullSize ());
    }

    static std::shared_ptr<RG::DescriptorBindableBufferResource> GPUBufferResourceCreator (const std::shared_ptr<RG::Operation>&, const GVK::ShaderModule&, const std::shared_ptr<SR::BufferObject>& bufferObject, bool&)
    {
        return std::make_unique<RG::GPUBufferResource> (bufferObject->GetFullSize ());
    }

public:

    UniformReflection (RG::ConnectionSet&     connectionSet,
                       const ResourceCreator& resourceCreator = &DefaultResourceCreator);

    void Flush (uint32_t frameIndex);

    ShaderKindSelector& operator[] (const RG::Operation& op);
    ShaderKindSelector& operator[] (const std::shared_ptr<RG::Operation>& op);
    ShaderKindSelector& operator[] (const GVK::UUID& opId);

    void PrintDebugInfo ();

private:

    // for transfering data between the two member functions
    std::vector<std::shared_ptr<RG::DescriptorBindableBufferResource>> bufferObjectResources;

    using BufferObjectConnection = std::tuple<std::shared_ptr<RG::Operation>, std::shared_ptr<SR::BufferObject>, std::shared_ptr<RG::DescriptorBindableBufferResource>, GVK::ShaderKind, bool>;
    std::vector<BufferObjectConnection> bufferObjectConnections;

    std::unordered_map<GVK::UUID, std::shared_ptr<SR::IBufferData>> udatas;

    void CreateGraphResources (const RG::ConnectionSet& connectionSet, const ResourceCreator& resourceCreator);

    void CreateGraphConnections (RG::ConnectionSet& connectionSet);

};


inline SR::IBufferData& UniformReflection::BufferObjectSelector::operator[] (std::string_view bufferObjectName)
{
    auto it = udatas.find (bufferObjectName);
    if (GVK_VERIFY (it != udatas.end ())) {
        return *it->second;
    }

    return SR::dummyBufferData;
}


inline void UniformReflection::BufferObjectSelector::Set (const std::string& bufferObjectName, const std::shared_ptr<SR::IBufferData>& bufferObjectData)
{
    udatas[bufferObjectName] = bufferObjectData;
}


inline bool UniformReflection::BufferObjectSelector::Contains (std::string_view bufferObjectName) const
{
    return udatas.find (bufferObjectName) != udatas.end ();
}


inline UniformReflection::BufferObjectSelector& UniformReflection::ShaderKindSelector::operator[] (GVK::ShaderKind shaderKind)
{
    auto it = bufferObjectSelectors.find (shaderKind);
    if (GVK_VERIFY (it != bufferObjectSelectors.end ())) {
        return it->second;
    }

    throw std::runtime_error ("no such shaderkind");
}


inline void UniformReflection::ShaderKindSelector::Set (GVK::ShaderKind shaderKind, UniformReflection::BufferObjectSelector&& bufferObjectSelector)
{
    bufferObjectSelectors[shaderKind] = std::move (bufferObjectSelector);
}


inline UniformReflection::ShaderKindSelector& UniformReflection::operator[] (const GVK::UUID& opId)
{
    GVK_ASSERT (selectors.find (opId) != selectors.end ());
    return selectors.at (opId);
}


inline UniformReflection::ShaderKindSelector& UniformReflection::operator[] (const RG::Operation& op)
{
    return (*this)[op.GetUUID ()];
}


inline UniformReflection::ShaderKindSelector& UniformReflection::operator[] (const std::shared_ptr<RG::Operation>& op)
{
    return (*this)[op->GetUUID ()];
}

} // namespace RG

#endif