#ifndef ACTIVEACTIONCONNECTOR_H
#define ACTIVEACTIONCONNECTOR_H

// Bridge to jp/mfapps/common/jni/ActiveActionConnector (Java side clears its downloaded-resource cache).
class ActiveActionConnector {
public:
    virtual ~ActiveActionConnector() {}
    virtual bool init();  // nothing to set up; true

    // Calls the static Java method ActiveActionConnector.jni_clearCache()V.
    static void clearCache();
};

#endif
