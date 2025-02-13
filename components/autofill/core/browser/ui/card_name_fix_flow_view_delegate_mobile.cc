// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/browser/ui/card_name_fix_flow_view_delegate_mobile.h"

#include <utility>

#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "components/grit/components_scaled_resources.h"
#include "components/strings/grit/components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace autofill {

CardNameFixFlowViewDelegateMobile::CardNameFixFlowViewDelegateMobile(
    const base::string16& inferred_cardholder_name,
    std::unique_ptr<base::DictionaryValue> legal_message,
    base::OnceCallback<void(const base::string16&)> upload_save_card_callback)
    : inferred_cardholder_name_(inferred_cardholder_name),
      upload_save_card_callback_(std::move(upload_save_card_callback)) {
  DCHECK(!upload_save_card_callback_.is_null());
  if (legal_message &&
      !LegalMessageLine::Parse(*legal_message, &legal_messages_,
                               /*escape_apostrophes=*/true)) {
    return;
  }
}

CardNameFixFlowViewDelegateMobile::~CardNameFixFlowViewDelegateMobile() {}

int CardNameFixFlowViewDelegateMobile::GetIconId() const {
  return IDR_AUTOFILL_GOOGLE_PAY_WITH_DIVIDER;
}

base::string16 CardNameFixFlowViewDelegateMobile::GetTitleText() const {
  return l10n_util::GetStringUTF16(
      IDS_AUTOFILL_SAVE_CARD_CARDHOLDER_NAME_FIX_FLOW_HEADER);
}

base::string16 CardNameFixFlowViewDelegateMobile::GetInferredCardHolderName()
    const {
  return inferred_cardholder_name_;
}

base::string16 CardNameFixFlowViewDelegateMobile::GetSaveButtonLabel() const {
  return l10n_util::GetStringUTF16(IDS_AUTOFILL_NAME_FIX_FLOW_PROMPT_SAVE_CARD);
}

void CardNameFixFlowViewDelegateMobile::Accept(const base::string16& name) {
  std::move(upload_save_card_callback_).Run(name);
}

}  // namespace autofill
