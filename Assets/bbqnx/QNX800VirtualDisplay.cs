using Unity.Collections;
using Unity.Collections.LowLevel.Unsafe;
using UnityEngine;
using System;
using System.Runtime.InteropServices;

[CreateAssetMenu(fileName = "QNX800VirtualDisplay", menuName = "bbqnx-test/QNX800 Virtual Screen Loader")]
public class QNX800VirtualDisplay : ScriptableObject
{
    [DllImport ("bbqnx/QNX800VirtualDisplayPlugin")]
    private static extern UInt64 NewQNX800VirtualDisplay(Int32 width, Int32 height);

    [DllImport ("bbqnx/QNX800VirtualDisplayPlugin")]
    private static extern void DelQNX800VirtualDisplay(UInt64 handle);
    
    [DllImport ("bbqnx/QNX800VirtualDisplayPlugin")]
    private static extern void ReadQNX800VirtualDisplay(UInt64 virtualDisplayHandle, Int32 width, Int32 height, UInt64 pixels);

    private UInt64 m_VirtualDisplayHandle = 0;
    private Texture2D m_TargetTex = null;

    public void NewVirtualDisplay(Int32 width, Int32 height)
    {
        m_VirtualDisplayHandle = NewQNX800VirtualDisplay(width, height);

        if(m_VirtualDisplayHandle != 0)
        {
            m_TargetTex = new Texture2D (width, height);
        }
    }

    public void OnDestroy()
    {
        DelQNX800VirtualDisplay(m_VirtualDisplayHandle);
    }

    public void ReadVirtualDisplay ()
    {
        NativeArray<Color32> pixels = m_TargetTex.GetRawTextureData<Color32>();
        unsafe 
        {
            ReadQNX800VirtualDisplay(m_VirtualDisplayHandle, 0, 0, (UInt64) pixels.GetUnsafePtr());
        }
    }

    public Texture2D GetScreenTexture()
    {
        return m_TargetTex;
    }
}

