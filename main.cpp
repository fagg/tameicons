//
// Copyright (c) 2023, Dr Ashton Fagg <ashton@fagg.id.au>
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

//
// This is a (pretty awful and hacky) util I wrote to deal with
// Windows not resetting your desktop icon positions after a
// resolution change (i.e. when disconnecting from an RDP
// session). This will let you dump the icon settings to a file, and
// then restore them later. It's warty, and I am kind of flying by the
// seat of my pants here as I don't really deal with COM or Win32 at
// all, so the code is probably pretty groaty but it works.
//

#define UNICODE
#define _UNICODE

#include <atlalloc.h>
#include <atlbase.h>
#include <atlcomcli.h>
#include <comdef.h>
#include <exdisp.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <stdio.h>
#include <wchar.h>
#include <windows.h>

#include <iostream>
#include <string>


//
// Returns a COM folder view of the desktop.
//

HRESULT GetDesktopFolderView(REFIID, void **);

//
// Restores the icon positions from the given file. This will
// handle flipping the auto-arrange/grid alignment settings
// off and on accordingly.
//

HRESULT RestoreIconPositionsFromFile(IFolderView *, PCWSTR);

//
// Get the FOLDERFLAGS for the given view, and do any toggling that
// might be required. The flag indicates whether we adjusted the flags,
// and the flags themselves are stored in the DWORD.
//

HRESULT SaveAndConfigureFolderFlags(IFolderView2 *, bool *, DWORD *);

//
// Reset the FOLDERFLAGS for the given view after we are done.
//

HRESULT ResetFolderFlagsToUser(IFolderView2 *, DWORD);

//
// Dumps the icon names and their positions to a file to be restored later.
//

HRESULT DumpIconPositionsToFile(IFolderView *, PCWSTR);

//
// Dumps the icon names and their positions to the console.
//

HRESULT EnumerateIcons(IFolderView *);

//
// Show a help message.
//

void ShowHelpMessage();

int __cdecl wmain(int argc, wchar_t **argv) {
  if (argc == 1 || argc > 3) {
    ShowHelpMessage();
    return 0;
  }

  bool dump_mode = false;
  bool restore_mode = false;
  bool show_mode = false;
  PCWSTR target_file;

  for (int i = 1; i < argc; ++i) {
    if (wcscmp(argv[i], L"/h")  == 0) {
      ShowHelpMessage();
      return 0;
    } else if (wcscmp(argv[i], L"/dump") == 0) {
      if (i + 1 < argc) {
        std::wcout << L"Dumping to file: " << argv[i+1] << L"\n";
        dump_mode = true;
        target_file = argv[i+1];
        break;
      } else {
        std::wcerr << L"Error: /dump requires a filename.\n";
        ShowHelpMessage();
        return -1;
      }
    } else if (wcscmp(argv[i], L"/restore") == 0) {
      if (i + 1 < argc) {
        std::wcout << L"Restoring from file: " << argv[i+1] << L"\n";
        restore_mode = true;
        target_file = argv[i+1];
        break;
      } else {
        std::wcerr << L"Error: /restore requires a filename.\n";
        ShowHelpMessage();
        return -1;
      }
    } else if (wcscmp(argv[i], L"/show") == 0) {
      show_mode = true;
      break;
    } else {
      std::wcerr << "Unknown options.\n";
      ShowHelpMessage();
      return -1;
    }
  }
  

  int rc = 0;
  HRESULT hr = CoInitialize(NULL);

  if (FAILED(hr)) {
    throw std::runtime_error("COM initialization failed");
  }

  CComPtr<IFolderView> folder_view;

  hr = GetDesktopFolderView(IID_PPV_ARGS(&folder_view));

  if (FAILED(hr)) {
    std::wcerr << L"Could not get a desktop folder view. Result = " << hr << std::endl;
    rc = -1;
    goto graceful_teardown;
  }

  if (show_mode) {
    hr = EnumerateIcons(folder_view);

    if (FAILED(hr)) {
      return -1;
    }

  } else if (dump_mode) {
    hr = DumpIconPositionsToFile(folder_view, target_file);

    if (FAILED(hr)) {
      return -1;
    }

  } else if (restore_mode) {

    //
    // These needs some special handling since apparently the API has changed?!
    //

    CComPtr<IFolderView2> folder_view2;
    hr = folder_view->QueryInterface(IID_IFolderView2, reinterpret_cast<void**>(&folder_view2));

    if (FAILED(hr)) {
      std::wcerr << L"Could not create IFolderView2\n";
      goto graceful_teardown;
    }


    bool flags_changed = false;
    DWORD user_flags;

    //
    // Get the users current settings, and configure them (if necessary)
    // so that we can fiddle with their icon layout.
    //

    hr = SaveAndConfigureFolderFlags(folder_view2, &flags_changed, &user_flags);

    if (FAILED(hr)) {
      rc = -1;
      goto graceful_teardown;
    }

    //
    // Now, perform the restore from the provided file.
    //

    hr = RestoreIconPositionsFromFile(folder_view, target_file);

    if (FAILED(hr)) {
      rc = -1;
      goto graceful_teardown;
    }

    //
    // If we changed the users settings, set them back.
    //
    if (flags_changed) {
      hr = ResetFolderFlagsToUser(folder_view2, user_flags);

      if (FAILED(hr)) {
        rc = -1;
        goto graceful_teardown;
      }
    }
  } else {
    std::wcout << L"This should never happen\n";
    return -1;
  }
  
graceful_teardown:
  CoUninitialize();
  return rc;
}

HRESULT GetDesktopFolderView(REFIID ref, void **ppv) {
  HRESULT hr;

  if (!ppv) {
    return E_INVALIDARG;
  }

  *ppv = nullptr;

  CComPtr<IShellWindows> shell;
  hr = shell.CoCreateInstance(CLSID_ShellWindows);

  if (FAILED(hr)) {
    return hr;
  }
  
  CComVariant vt_loc(CSIDL_DESKTOP);
  CComVariant vt_empty;
  long handle = 0;
  CComPtr<IDispatch> dispatch;

  hr = shell->FindWindowSW(&vt_loc, 
                           &vt_empty, 
                           SWC_DESKTOP,
                           &handle, 
                           SWFO_NEEDDISPATCH, 
                           &dispatch);
  
  if (FAILED(hr)) {
    return hr;
  }

  CComPtr<IShellBrowser> shell_browser;
  hr = CComQIPtr<IServiceProvider>(dispatch)->QueryService(SID_STopLevelBrowser, 
                                                           IID_PPV_ARGS(&shell_browser));

  if (FAILED(hr)) {
    return hr;
  }
  
  CComPtr<IShellView> shell_view;
  hr = shell_browser->QueryActiveShellView(&shell_view);

  if (FAILED(hr)) {
    return hr;
  }

  return shell_view->QueryInterface(ref, ppv);
}

HRESULT EnumerateIcons(IFolderView *view) {
  if (!view) {
    throw std::runtime_error("Invalid view pointer.");
  }

  CComPtr<IEnumIDList> idl;
  HRESULT hr = view->Items(SVGIO_ALLVIEW, IID_PPV_ARGS(&idl));

  if (FAILED(hr)) {
    std::wcerr << "Could not get item enumerator\n";
    return hr;
  }


  CComPtr<IShellFolder> folder;
  hr = view->GetFolder(IID_PPV_ARGS(&folder));

  if (FAILED(hr)) {
    std::wcerr << "Could not get folder.\n";
    return hr;
  }

  CComHeapPtr<ITEMID_CHILD> spidl;
  for (; idl->Next(1, &spidl, nullptr) == S_OK; spidl.Free()) {
    STRRET name_stret;
    CComHeapPtr<wchar_t> name_str;
    hr = folder->GetDisplayNameOf(spidl, SHGDN_NORMAL, &name_stret);

    if (FAILED(hr)) {
      std::wcerr << "Could not get display name.\n";
      return hr;
    }
    
    hr = StrRetToStr(&name_stret, spidl, &name_str);
    if (FAILED(hr)) {
      std::wcerr << "String conversion error\n";
      return hr;
    }

    POINT pt;
    hr = view->GetItemPosition(spidl, &pt);

    if (FAILED(hr)) {
      std::wcerr << "Could not get icon position.\n";
      return hr;
    }

    std::wcout << L"Name = " << name_str.m_pData 
               << L", Pos = (" << pt.x << L"," << pt.y << L")" << std::endl;  
  }

  return S_OK;
}

HRESULT DumpIconPositionsToFile(IFolderView *view, PCWSTR file) {
  if (!view) {
    throw std::runtime_error("Invalid view pointer.");
  }

  CComPtr<IStream> stream;
  HRESULT hr = SHCreateStreamOnFileEx(file, 
                                      STGM_CREATE | STGM_WRITE,
                                      FILE_ATTRIBUTE_NORMAL,
                                      TRUE,
                                      nullptr,
                                      &stream);

  if (FAILED(hr)) {
    std::wcerr << L"Could not open output file stream.\n";
    return hr;
  }

  CComPtr<IEnumIDList> idl;
  hr = view->Items(SVGIO_ALLVIEW, IID_PPV_ARGS(&idl));

  if (FAILED(hr)) {
    std::wcerr << "Could not get item enumerator\n";
    return hr;
  }

  CComPtr<IShellFolder> folder;
  hr = view->GetFolder(IID_PPV_ARGS(&folder));

  if (FAILED(hr)) {
    std::wcerr << "Could not get folder.\n";
    return hr;
  }

  CComHeapPtr<ITEMID_CHILD> spidl;
  for (; idl->Next(1, &spidl, nullptr) == S_OK; spidl.Free()) {
    POINT pt;
    hr = view->GetItemPosition(spidl, &pt);

    if (FAILED(hr)) {
      std::wcerr << L"Could not get icon position.\n";
      return hr;
    }

    hr = IStream_WritePidl(stream, spidl);
    if (FAILED(hr)) {
      std::wcerr << L"IStream_WritePidl failed.\n";
      return hr;
    }

    hr = IStream_Write(stream, &pt, sizeof(pt));
    if (FAILED(hr)) {
      std::wcerr << L"IStream_Write failed.\n";
      return hr;
    }
  }
  
  return hr;
}

HRESULT RestoreIconPositionsFromFile(IFolderView *view, PCWSTR file) {
  if (!view) {
    throw std::runtime_error("Invalid view pointer.");
  }
  
  CComPtr<IStream> stream;
  HRESULT hr = SHCreateStreamOnFileEx(file, 
                                      STGM_READ,
                                      FILE_ATTRIBUTE_NORMAL,
                                      FALSE,
                                      nullptr,
                                      &stream);

  if (FAILED(hr)) {
    std::wcerr << L"Could not open file.\n";
    return hr;
  }
  
  CComHeapPtr<ITEMID_CHILD> spidl;
  POINT pt;

  for (; IStream_ReadPidl(stream, &spidl) == S_OK &&
         IStream_Read(stream, &pt, sizeof(pt)) == S_OK;
       spidl.Free()) {
    PCITEMID_CHILD apidl[1] = {spidl};
    hr = view->SelectAndPositionItems(1, apidl, &pt, SVSI_POSITIONITEM);

    if (FAILED(hr)) {
      std::wcerr << "Could not position icon.\n";
      return hr;
    }
  }

  return hr;
}

HRESULT SaveAndConfigureFolderFlags(IFolderView2 *view, bool *changed_flags, DWORD *user_flags) {
  if ((!view) || (!changed_flags) || (!user_flags)) {
    throw std::runtime_error("SaveAndConfigureFolderFlags got an invalid pointer.");
  }

  HRESULT hr = view->GetCurrentFolderFlags(user_flags);
  if (FAILED(hr)) {
    std::wcerr << "Failed to configure folder flags.\n";
    return hr;
  }

  //
  // Save the current state.
  //

  const bool auto_arr = ((*user_flags) & FWF_AUTOARRANGE) != 0;
  const bool grid_snap = ((*user_flags) & FWF_SNAPTOGRID) != 0;

  DWORD flags = *user_flags;
  *changed_flags = (auto_arr | grid_snap);

  if (auto_arr) {
    flags &= ~FWF_AUTOARRANGE;
  }

  if (grid_snap) {
    flags &= ~FWF_SNAPTOGRID;
  }
  
  if (*changed_flags) {
    hr = view->SetCurrentFolderFlags(flags, flags);
  }

  if (FAILED(hr)) {
    std::wcerr << L"Could not set folder flags.\n";
  }

  return hr;
}

HRESULT ResetFolderFlagsToUser(IFolderView2 *view, DWORD user_flags) {
  if ((!view)) {
    throw std::runtime_error("ResetFolderFlagsToUser got an invalid pointer.");
  }

  HRESULT hr = view->SetCurrentFolderFlags(user_flags, user_flags);

  if (FAILED(hr)) {
    std::wcerr << L"Failed to reset folder flags to user prefs.\n";
  }

  return hr;
}

void ShowHelpMessage() {
  std::wcout << L"Usage:\n";
  std::wcout << L"/h                   Displays this helpful message.\n";
  std::wcout << L"/show                Shows the current icons and their positions.\n";
  std::wcout << L"/dump <filename>     Dump the current icon state to a file.\n";
  std::wcout << L"/restore <filename>  Restore the icon state from a file.\n";
}
