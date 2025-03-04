using UnityEngine;
using System;
using System.Runtime.InteropServices;

[CreateAssetMenu(fileName = "QNX800VirtualDisplayLoader", menuName = "bbqnx-test/QNX800 Virtual Screen Loader")]
public class QNX800VirtualDisplayLoader : ScriptableObject
{
    [DllImport ("bbqnx/QNX800VirtualDisplayPlugin")]
    private static extern UInt64 NewQNX800VirtualDisplay (UInt32 width, UInt32 height);

    [DllImport ("bbqnx/QNX800VirtualDisplayPlugin")]
    private static extern void DelQNX800VirtualDisplay (UInt64 handle);
    
    [DllImport ("bbqnx/QNX800VirtualDisplayPlugin")]
    private static extern UInt64 ReadQNX800VirtualDisplay (UInt64 VirtualDisplayHandle, UInt32 GLTexHandle);

    private UInt64 m_VirtualDisplayHandle = 0;

    public void NewVirtualDisplay (UInt32 width, UInt32 height)
    {
        if(m_VirtualDisplayHandle != 0)
        {
            m_VirtualDisplayHandle = NewQNX800VirtualDisplay (width, height);
        }
    }

    public void OnDestroy ()
    {
        DelQNX800VirtualDisplay (m_VirtualDisplayHandle);
    }

    public UInt64 ReadVirtualDisplay (UInt32 width, UInt32 height)
    {
        return ReadQNX800VirtualDisplay (m_VirtualDisplayHandle, 78);
    }
}

