// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/locale/locale_feature_pod_controller.h"

#include "ash/resources/vector_icons/vector_icons.h"
#include "ash/shell.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/system/model/system_tray_model.h"
#include "ash/system/unified/feature_pod_button.h"
#include "ash/system/unified/unified_system_tray_controller.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/base/l10n/l10n_util.h"

namespace ash {

LocaleFeaturePodController::LocaleFeaturePodController(
    UnifiedSystemTrayController* tray_controller)
    : tray_controller_(tray_controller) {}

LocaleFeaturePodController::~LocaleFeaturePodController() = default;

FeaturePodButton* LocaleFeaturePodController::CreateButton() {
  auto* button = new FeaturePodButton(this);
  const bool visible =
      !Shell::Get()->system_tray_model()->locale_list().empty();
  button->SetVisible(visible);
  if (visible) {
    button->SetVectorIcon(kUnifiedMenuLocaleIcon);
    button->SetIconAndLabelTooltips(
        l10n_util::GetStringUTF16(IDS_ASH_STATUS_TRAY_LOCALE_TOOLTIP));
    button->SetLabel(l10n_util::GetStringUTF16(IDS_ASH_STATUS_TRAY_LOCALE));
    button->ShowDetailedViewArrow();
    button->DisableLabelButtonFocus();
    button->SetSubLabel(base::UTF8ToUTF16(
        Shell::Get()->system_tray_model()->current_locale_iso_code()));
  }
  return button;
}

void LocaleFeaturePodController::OnIconPressed() {
  tray_controller_->ShowLocaleDetailedView();
}

SystemTrayItemUmaType LocaleFeaturePodController::GetUmaType() const {
  return SystemTrayItemUmaType::UMA_LOCALE;
}

}  // namespace ash
