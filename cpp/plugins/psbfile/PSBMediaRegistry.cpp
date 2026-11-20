#include "PSBMediaRegistry.h"

#include <unordered_map>

#include "PSBMedia.h"
#include "PSBValue.h"
#include "StorageIntf.h"

#include "log/TVPLog.h"

namespace PSB {
    namespace {
        PSBMedia *psbMedia = nullptr;
        std::unordered_map<std::string, int> containerRefCounts;

        void registerValueResources(const ttstr &normalizedContainer,
                                    const std::shared_ptr<IPSBValue> &value,
                                    std::vector<std::string> &path) {
            if(psbMedia == nullptr || value == nullptr) {
                return;
            }

            if(const auto resource =
                   std::dynamic_pointer_cast<PSBResource>(value)) {
                ttstr resourceKey;
                for(size_t index = 0; index < path.size(); ++index) {
                    if(index != 0) {
                        resourceKey += TJS_W("/");
                    }
                    resourceKey += ttstr{ path[index] };
                }
                if(resourceKey.IsEmpty()) {
                    return;
                }
                psbMedia->NormalizePathName(resourceKey);
                psbMedia->add((normalizedContainer + TJS_W("/") + resourceKey)
                                  .AsStdString(),
                              resource);
                return;
            }

            if(const auto dic =
                   std::dynamic_pointer_cast<PSBDictionary>(value)) {
                for(const auto &[key, child] : *dic) {
                    path.push_back(key);
                    registerValueResources(normalizedContainer, child, path);
                    path.pop_back();
                }
                return;
            }

            if(const auto list = std::dynamic_pointer_cast<PSBList>(value)) {
                for(size_t index = 0; index < list->size(); ++index) {
                    path.push_back(std::to_string(index));
                    registerValueResources(normalizedContainer,
                                           (*list)[static_cast<int>(index)],
                                           path);
                    path.pop_back();
                }
            }
        }

        void registerRootResourcesForContainer(
            const ttstr &container,
            const std::shared_ptr<const PSBDictionary> &root) {
            if(psbMedia == nullptr || root == nullptr || container.IsEmpty()) {
                return;
            }

            ttstr normalizedContainer = container;
            psbMedia->NormalizeDomainName(normalizedContainer);
            ++containerRefCounts[normalizedContainer.AsStdString()];

            std::vector<std::string> path;
            registerValueResources(normalizedContainer,
                                   std::const_pointer_cast<PSBDictionary>(root),
                                   path);
        }

        void unregisterRootResourcesForContainer(const ttstr &container) {
            if(psbMedia == nullptr || container.IsEmpty()) {
                return;
            }

            ttstr normalizedContainer = container;
            psbMedia->NormalizeDomainName(normalizedContainer);
            const auto key = normalizedContainer.AsStdString();
            const auto it = containerRefCounts.find(key);
            if(it != containerRefCounts.end()) {
                if(--it->second > 0) {
                    return;
                }
                containerRefCounts.erase(it);
            }
            psbMedia->removeByPrefix(key + "/");
        }
    } // namespace

    void initPSBMedia() {
        if(psbMedia != nullptr) {
            return;
        }

        psbMedia = new PSBMedia();
        TVPRegisterStorageMedia(psbMedia);
        psbMedia->Release();
        G_PluginLog.info("initPsbFile");
    }

    void deInitPSBMedia() {
        containerRefCounts.clear();
        if(psbMedia != nullptr) {
            TVPUnregisterStorageMedia(psbMedia);
            psbMedia = nullptr;
        }
        G_PluginLog.info("deInitPsbFile");
    }

    void
    registerRootResources(const ttstr &container,
                          const std::shared_ptr<const PSBDictionary> &root) {
        initPSBMedia();
        registerRootResourcesForContainer(container, root);
    }

    void
    registerRootResources(const std::vector<ttstr> &containers,
                          const std::shared_ptr<const PSBDictionary> &root) {
        initPSBMedia();
        for(const auto &container : containers) {
            registerRootResourcesForContainer(container, root);
        }
    }

    void registerRootResources(const ttstr &container, const PSBFile &file) {
        registerRootResources(container, file.getObjects());
    }

    void registerRootResources(const std::vector<ttstr> &containers,
                               const PSBFile &file) {
        registerRootResources(containers, file.getObjects());
    }

    void unregisterRootResources(const ttstr &container) {
        unregisterRootResourcesForContainer(container);
    }

    void unregisterRootResources(const std::vector<ttstr> &containers) {
        for(const auto &container : containers) {
            unregisterRootResourcesForContainer(container);
        }
    }
} // namespace PSB
