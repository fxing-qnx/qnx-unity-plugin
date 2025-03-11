using Unity.Collections;
using Unity.Collections.LowLevel.Unsafe;
using UnityEngine;
using System;
using System.Runtime.InteropServices;

[CreateAssetMenu(fileName = "QNX800WindowRecorder", menuName = "bbqnx-test/QNX800 window recorder")]
public class QNX800WindowRecorder : ScriptableObject
{
    [DllImport ("bbqnx/QNX800WindowRecorder")]
    private static extern UInt64 NewQNX800WindowRecorder(Int32 width, Int32 height);

    [DllImport ("bbqnx/QNX800WindowRecorder")]
    private static extern void DelQNX800WindowRecorder(UInt64 handle);
    
    [DllImport ("bbqnx/QNX800WindowRecorder")]
    private static extern void ReadQNX800WindowRecorder(UInt64 virtualDisplayHandle, Int32 width, Int32 height, UInt64 pixels);

    private UInt64 m_VirtualDisplayHandle = 0;
    private Texture2D m_TargetTex = null;
}

