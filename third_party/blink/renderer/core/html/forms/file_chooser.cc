/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/html/forms/file_chooser.h"

#include "services/service_manager/public/cpp/interface_provider.h"
#include "third_party/blink/public/platform/file_path_conversion.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/page/chrome_client_impl.h"

namespace blink {

using mojom::blink::FileChooserFileInfo;
using mojom::blink::FileChooserFileInfoPtr;
using mojom::blink::NativeFileInfo;

FileChooserClient::~FileChooserClient() = default;

FileChooser* FileChooserClient::NewFileChooser(
    const mojom::blink::FileChooserParams& params) {
  if (chooser_)
    chooser_->DisconnectClient();

  chooser_ = FileChooser::Create(this, params);
  return chooser_.get();
}

void FileChooserClient::DisconnectFileChooser() {
  DCHECK(HasConnectedFileChooser());
  chooser_->DisconnectClient();
}

inline FileChooser::FileChooser(FileChooserClient* client,
                                const mojom::blink::FileChooserParams& params)
    : client_(client), params_(params.Clone()) {}

scoped_refptr<FileChooser> FileChooser::Create(
    FileChooserClient* client,
    const mojom::blink::FileChooserParams& params) {
  return base::AdoptRef(new FileChooser(client, params));
}

FileChooser::~FileChooser() = default;

bool FileChooser::OpenFileChooser(ChromeClientImpl& chrome_client_impl) {
  LocalFrame* frame = FrameOrNull();
  if (!frame)
    return false;
  chrome_client_impl_ = chrome_client_impl;
  frame->GetInterfaceProvider().GetInterface(&file_chooser_);
  file_chooser_.set_connection_error_handler(
      WTF::Bind(&FileChooser::DidCloseChooser, WTF::Unretained(this)));
  file_chooser_->OpenFileChooser(
      params_.Clone(),
      WTF::Bind(&FileChooser::DidChooseFiles, WTF::Unretained(this)));

  // Should be released on file choosing or connection error.
  AddRef();
  chrome_client_impl.RegisterPopupOpeningObserver(client_);
  return true;
}

void FileChooser::EnumerateChosenDirectory() {
  DCHECK_EQ(params_->selected_files.size(), 1u);
  LocalFrame* frame = FrameOrNull();
  if (!frame)
    return;
  DCHECK(!chrome_client_impl_);
  frame->GetInterfaceProvider().GetInterface(&file_chooser_);
  file_chooser_.set_connection_error_handler(
      WTF::Bind(&FileChooser::DidCloseChooser, WTF::Unretained(this)));
  file_chooser_->EnumerateChosenDirectory(
      std::move(params_->selected_files[0]),
      WTF::Bind(&FileChooser::DidChooseFiles, WTF::Unretained(this)));

  // Should be released on file choosing or connection error.
  AddRef();
}

void FileChooser::DidChooseFiles(mojom::blink::FileChooserResultPtr result) {
  DCHECK(result);
  FileChooserFileInfoList files = std::move(result->files);
  // FIXME: This is inelegant. We should not be looking at params_ here.
  if (params_->selected_files.size() == files.size()) {
    bool was_changed = false;
    for (unsigned i = 0; i < files.size(); ++i) {
      // TODO(tkent): If a file system URL was already selected, and new
      // chooser session selects the same one, a |change| event is
      // dispatched unexpectedly.
      // |selected_files| is created by FileList::
      // PathsForUserVisibleFiles(), and it returns File::name() for
      // file system URLs. Comparing File::name() doesn't make
      // sense. We should compare file system URLs.
      if (!files[i]->is_native_file() ||
          params_->selected_files[i] !=
              files[i]->get_native_file()->file_path) {
        was_changed = true;
        break;
      }
    }
    if (!was_changed) {
      DidCloseChooser();
      return;
    }
  }

  if (client_)
    client_->FilesChosen(std::move(files), result->base_directory);
  DidCloseChooser();
}

void FileChooser::DidCloseChooser() {
  // Close the binding explicitly to avoid this function is called again as a
  // connection error handler.
  file_chooser_.reset();

  // Some cleanup for OpenFileChooser() path.
  if (chrome_client_impl_) {
    chrome_client_impl_->DidCompleteFileChooser(*this);
    if (client_)
      chrome_client_impl_->UnregisterPopupOpeningObserver(client_);
  }

  Release();
}

FileChooserFileInfoPtr CreateFileChooserFileInfoNative(
    const String& path,
    const String& display_name) {
  return FileChooserFileInfo::NewNativeFile(
      NativeFileInfo::New(StringToFilePath(path), display_name));
}

FileChooserFileInfoPtr CreateFileChooserFileInfoFileSystem(
    const KURL& url,
    base::Time modification_time,
    int64_t length) {
  return FileChooserFileInfo::NewFileSystem(
      mojom::blink::FileSystemFileInfo::New(url, modification_time, length));
}

}  // namespace blink
