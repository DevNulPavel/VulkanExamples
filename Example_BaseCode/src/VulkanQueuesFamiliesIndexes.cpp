#include "VulkanQueuesFamiliesIndexes.h"


VulkanQueuesFamiliesIndexes::VulkanQueuesFamiliesIndexes(){
    renderQueuesFamilyIndex = -1;
    renderQueuesFamilyQueuesCount = 0;
    renderQueuesTimeStampValidBits = 0;
    presentQueuesFamilyIndex = -1;
    presentQueuesFamilyQueuesCount = 0;
    presentQueuesTimeStampValidBits = 0;
}

bool VulkanQueuesFamiliesIndexes::isComplete() const{
    return (renderQueuesFamilyIndex >= 0) && (presentQueuesFamilyIndex >= 0);
}
