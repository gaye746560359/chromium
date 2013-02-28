// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/google_apis/drive_api_operations.h"

#include "base/bind.h"
#include "base/json/json_writer.h"
#include "base/values.h"
#include "chrome/browser/google_apis/drive_api_parser.h"
#include "chrome/browser/google_apis/operation_util.h"
#include "content/public/browser/browser_thread.h"

using content::BrowserThread;

namespace google_apis {
namespace {

const char kContentTypeApplicationJson[] = "application/json";
const char kDirectoryMimeType[] = "application/vnd.google-apps.folder";

// Parses the JSON value to AboutResource and runs |callback| on the UI
// thread once parsing is done.
void ParseAboutResourceAndRun(
    const GetAboutResourceCallback& callback,
    GDataErrorCode error,
    scoped_ptr<base::Value> value) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(!callback.is_null());

  scoped_ptr<AboutResource> about_resource;
  if (value.get()) {
    about_resource = AboutResource::CreateFrom(*value);
    if (!about_resource) {
      // Failed to parse the JSON value (although the JSON value is available),
      // so let callback know the parsing error.
      error = GDATA_PARSE_ERROR;
    }
  }

  callback.Run(error, about_resource.Pass());
}

}  // namespace

//============================== GetAboutOperation =============================

GetAboutOperation::GetAboutOperation(
    OperationRegistry* registry,
    net::URLRequestContextGetter* url_request_context_getter,
    const DriveApiUrlGenerator& url_generator,
    const GetAboutResourceCallback& callback)
    : GetDataOperation(registry, url_request_context_getter,
                       base::Bind(&ParseAboutResourceAndRun, callback)),
      url_generator_(url_generator) {
  DCHECK(!callback.is_null());
}

GetAboutOperation::~GetAboutOperation() {}

GURL GetAboutOperation::GetURL() const {
  return url_generator_.GetAboutUrl();
}

//============================== GetApplistOperation ===========================

GetApplistOperation::GetApplistOperation(
    OperationRegistry* registry,
    net::URLRequestContextGetter* url_request_context_getter,
    const DriveApiUrlGenerator& url_generator,
    const GetDataCallback& callback)
    : GetDataOperation(registry, url_request_context_getter, callback),
      url_generator_(url_generator) {
  DCHECK(!callback.is_null());
}

GetApplistOperation::~GetApplistOperation() {}

GURL GetApplistOperation::GetURL() const {
  return url_generator_.GetApplistUrl();
}

//============================ GetChangelistOperation ==========================

GetChangelistOperation::GetChangelistOperation(
    OperationRegistry* registry,
    net::URLRequestContextGetter* url_request_context_getter,
    const DriveApiUrlGenerator& url_generator,
    const GURL& url,
    int64 start_changestamp,
    const GetDataCallback& callback)
    : GetDataOperation(registry, url_request_context_getter, callback),
      url_generator_(url_generator),
      url_(url),
      start_changestamp_(start_changestamp) {
  DCHECK(!callback.is_null());
}

GetChangelistOperation::~GetChangelistOperation() {}

GURL GetChangelistOperation::GetURL() const {
  return url_generator_.GetChangelistUrl(url_, start_changestamp_);
}

//============================= GetFlielistOperation ===========================

GetFilelistOperation::GetFilelistOperation(
    OperationRegistry* registry,
    net::URLRequestContextGetter* url_request_context_getter,
    const DriveApiUrlGenerator& url_generator,
    const GURL& url,
    const std::string& search_string,
    const GetDataCallback& callback)
    : GetDataOperation(registry, url_request_context_getter, callback),
      url_generator_(url_generator),
      url_(url),
      search_string_(search_string) {
  DCHECK(!callback.is_null());
}

GetFilelistOperation::~GetFilelistOperation() {}

GURL GetFilelistOperation::GetURL() const {
  return url_generator_.GetFilelistUrl(url_, search_string_);
}

//=============================== GetFlieOperation =============================

GetFileOperation::GetFileOperation(
    OperationRegistry* registry,
    net::URLRequestContextGetter* url_request_context_getter,
    const DriveApiUrlGenerator& url_generator,
    const std::string& file_id,
    const GetDataCallback& callback)
    : GetDataOperation(registry, url_request_context_getter, callback),
      url_generator_(url_generator),
      file_id_(file_id) {
  DCHECK(!callback.is_null());
}

GetFileOperation::~GetFileOperation() {}

GURL GetFileOperation::GetURL() const {
  return url_generator_.GetFileUrl(file_id_);
}

namespace drive {

//========================== CreateDirectoryOperation ==========================

CreateDirectoryOperation::CreateDirectoryOperation(
    OperationRegistry* registry,
    net::URLRequestContextGetter* url_request_context_getter,
    const DriveApiUrlGenerator& url_generator,
    const std::string& parent_resource_id,
    const std::string& directory_name,
    const GetDataCallback& callback)
    : GetDataOperation(registry, url_request_context_getter, callback),
      url_generator_(url_generator),
      parent_resource_id_(parent_resource_id),
      directory_name_(directory_name) {
  DCHECK(!callback.is_null());
}

CreateDirectoryOperation::~CreateDirectoryOperation() {}

GURL CreateDirectoryOperation::GetURL() const {
  if (parent_resource_id_.empty() || directory_name_.empty()) {
    return GURL();
  }
  return url_generator_.GetFilelistUrl(GURL(), "");
}

net::URLFetcher::RequestType CreateDirectoryOperation::GetRequestType() const {
  return net::URLFetcher::POST;
}

bool CreateDirectoryOperation::GetContentData(std::string* upload_content_type,
                                              std::string* upload_content) {
  *upload_content_type = kContentTypeApplicationJson;

  base::DictionaryValue root;
  root.SetString("title", directory_name_);
  {
    base::DictionaryValue* parent_value = new base::DictionaryValue;
    parent_value->SetString("id", parent_resource_id_);
    base::ListValue* parent_list_value = new base::ListValue;
    parent_list_value->Append(parent_value);
    root.Set("parents", parent_list_value);
  }
  root.SetString("mimeType", kDirectoryMimeType);

  base::JSONWriter::Write(&root, upload_content);

  DVLOG(1) << "CreateDirectory data: " << *upload_content_type << ", ["
           << *upload_content << "]";
  return true;
}

//=========================== RenameResourceOperation ==========================

RenameResourceOperation::RenameResourceOperation(
    OperationRegistry* registry,
    net::URLRequestContextGetter* url_request_context_getter,
    const DriveApiUrlGenerator& url_generator,
    const std::string& resource_id,
    const std::string& new_name,
    const EntryActionCallback& callback)
    : EntryActionOperation(registry, url_request_context_getter, callback),
      url_generator_(url_generator),
      resource_id_(resource_id),
      new_name_(new_name) {
  DCHECK(!callback.is_null());
}

RenameResourceOperation::~RenameResourceOperation() {}

net::URLFetcher::RequestType RenameResourceOperation::GetRequestType() const {
  return net::URLFetcher::PATCH;
}

std::vector<std::string>
RenameResourceOperation::GetExtraRequestHeaders() const {
  std::vector<std::string> headers;
  headers.push_back(util::kIfMatchAllHeader);
  return headers;
}

GURL RenameResourceOperation::GetURL() const {
  return url_generator_.GetFileUrl(resource_id_);
}

bool RenameResourceOperation::GetContentData(std::string* upload_content_type,
                                             std::string* upload_content) {
  *upload_content_type = kContentTypeApplicationJson;

  base::DictionaryValue root;
  root.SetString("title", new_name_);
  base::JSONWriter::Write(&root, upload_content);

  DVLOG(1) << "RenameResource data: " << *upload_content_type << ", ["
           << *upload_content << "]";
  return true;
}

//=========================== TrashResourceOperation ===========================

TrashResourceOperation::TrashResourceOperation(
    OperationRegistry* registry,
    net::URLRequestContextGetter* url_request_context_getter,
    const DriveApiUrlGenerator& url_generator,
    const std::string& resource_id,
    const EntryActionCallback& callback)
    : EntryActionOperation(registry, url_request_context_getter, callback),
      url_generator_(url_generator),
      resource_id_(resource_id) {
  DCHECK(!callback.is_null());
}

TrashResourceOperation::~TrashResourceOperation() {}

GURL TrashResourceOperation::GetURL() const {
  return url_generator_.GetFileTrashUrl(resource_id_);
}

net::URLFetcher::RequestType TrashResourceOperation::GetRequestType() const {
  return net::URLFetcher::POST;
}

//========================== InsertResourceOperation ===========================

InsertResourceOperation::InsertResourceOperation(
    OperationRegistry* registry,
    net::URLRequestContextGetter* url_request_context_getter,
    const DriveApiUrlGenerator& url_generator,
    const std::string& parent_resource_id,
    const std::string& resource_id,
    const EntryActionCallback& callback)
    : EntryActionOperation(registry, url_request_context_getter, callback),
      url_generator_(url_generator),
      parent_resource_id_(parent_resource_id),
      resource_id_(resource_id) {
  DCHECK(!callback.is_null());
}

InsertResourceOperation::~InsertResourceOperation() {}

GURL InsertResourceOperation::GetURL() const {
  return url_generator_.GetChildrenUrl(parent_resource_id_);
}

net::URLFetcher::RequestType InsertResourceOperation::GetRequestType() const {
  return net::URLFetcher::POST;
}

bool InsertResourceOperation::GetContentData(std::string* upload_content_type,
                                             std::string* upload_content) {
  *upload_content_type = kContentTypeApplicationJson;

  base::DictionaryValue root;
  root.SetString("id", resource_id_);
  base::JSONWriter::Write(&root, upload_content);

  DVLOG(1) << "InsertResource data: " << *upload_content_type << ", ["
           << *upload_content << "]";
  return true;
}

//========================== DeleteResourceOperation ===========================

DeleteResourceOperation::DeleteResourceOperation(
    OperationRegistry* registry,
    net::URLRequestContextGetter* url_request_context_getter,
    const DriveApiUrlGenerator& url_generator,
    const std::string& parent_resource_id,
    const std::string& resource_id,
    const EntryActionCallback& callback)
    : EntryActionOperation(registry, url_request_context_getter, callback),
      url_generator_(url_generator),
      parent_resource_id_(parent_resource_id),
      resource_id_(resource_id) {
  DCHECK(!callback.is_null());
}

DeleteResourceOperation::~DeleteResourceOperation() {}

GURL DeleteResourceOperation::GetURL() const {
  return url_generator_.GetChildrenUrlForRemoval(
      parent_resource_id_, resource_id_);
}

net::URLFetcher::RequestType DeleteResourceOperation::GetRequestType() const {
  return net::URLFetcher::DELETE_REQUEST;
}

}  // namespace drive
}  // namespace google_apis
