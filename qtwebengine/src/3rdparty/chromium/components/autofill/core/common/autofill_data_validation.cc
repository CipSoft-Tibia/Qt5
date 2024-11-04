// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/common/autofill_data_validation.h"

#include "base/ranges/algorithm.h"
#include "components/autofill/core/common/autofill_constants.h"
#include "components/autofill/core/common/form_data.h"
#include "components/autofill/core/common/form_field_data.h"
#include "components/autofill/core/common/password_form_fill_data.h"
#include "url/gurl.h"

namespace autofill {

bool IsValidString(const std::string& str) {
  return str.size() <= kMaxStringLength;
}

bool IsValidString16(const std::u16string& str) {
  return str.size() <= kMaxStringLength;
}

bool IsValidGURL(const GURL& url) {
  return url.is_empty() || url.is_valid();
}

bool IsValidOption(const SelectOption& option) {
  return IsValidString16(option.content) && IsValidString16(option.value);
}

bool IsValidFormFieldData(const FormFieldData& field) {
  return IsValidString16(field.label) && IsValidString16(field.name) &&
         IsValidString16(field.value) &&
         IsValidString(field.form_control_type) &&
         IsValidString(field.autocomplete_attribute) &&
         IsValidOptionVector(field.options);
}

bool IsValidFormData(const FormData& form) {
  return IsValidString16(form.name) && IsValidGURL(form.url) &&
         IsValidGURL(form.action) && form.fields.size() <= kMaxListSize &&
         base::ranges::all_of(form.fields, &IsValidFormFieldData);
}

bool IsValidPasswordFormFillData(const PasswordFormFillData& form) {
  return IsValidGURL(form.url) &&
         IsValidString16(form.preferred_login.username_value) &&
         IsValidString16(form.preferred_login.password_value) &&
         IsValidString(form.preferred_login.realm) &&
         base::ranges::all_of(form.additional_logins, [](const auto& login) {
           return IsValidString16(login.username_value) &&
                  IsValidString16(login.password_value) &&
                  IsValidString(login.realm);
         });
}

bool IsValidOptionVector(const base::span<const SelectOption>& options) {
  if (options.size() > kMaxListSize)
    return false;
  for (const auto& option : options) {
    if (!IsValidString16(option.content))
      return false;
  }
  return true;
//   return options.size() <= kMaxListSize &&
//          base::ranges::all_of(options, &IsValidString16,
//                               &SelectOption::content);
}

bool IsValidString16Vector(const base::span<const std::u16string>& strings) {
  if (strings.size() > kMaxListSize)
    return false;
  for (const auto& i : strings) {
    if (!IsValidString16(i))
      return false;
  }
  return true;
//   return v.size() <= kMaxListSize && base::ranges::all_of(v, &IsValidString16);
}

bool IsValidFormDataVector(const base::span<const FormData>& forms) {
  if (forms.size() > kMaxListSize)
    return false;
  for (const auto& i : forms) {
    if (!IsValidFormData(i))
      return false;
  }
  return true;
//   return v.size() <= kMaxListSize && base::ranges::all_of(v, &IsValidFormData);
}

}  // namespace autofill
