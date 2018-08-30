#include <jni.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

AAssetManager *g_androidAssetMgr = nullptr;

extern "C" JNIEXPORT void JNICALL Java_pl_kondrak_quakebspviewer_initAssetManager(JNIEnv *env, jobject obj, jobject assetManager)
{
    g_androidAssetMgr = AAssetManager_fromJava(env, assetManager);
}
