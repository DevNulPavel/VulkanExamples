#include "VulkanQueuesFamiliesIndexes.h"


VulkanQueuesFamiliesIndexes::VulkanQueuesFamiliesIndexes(){
    renderQueueFamilyIndex = -1;
    renderQueueFamilyQueuesCount = 0;
    presentQueueFamilyIndex = -1;
    presentQueueFamilyQueuesCount = 0;
}

bool VulkanQueuesFamiliesIndexes::isComplete() const{
    return (renderQueueFamilyIndex >= 0) && (presentQueueFamilyIndex >= 0);
}
