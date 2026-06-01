# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import logging
import urllib.error
import urllib.parse as urlparse
import urllib.request
from typing import Dict, Union


def urlopen(url: str, timeout: Union[int, float] = 10):
  try:
    logging.debug("Opening url: %s", url)
    return urllib.request.urlopen(url, timeout=timeout)
  except (urllib.error.HTTPError, urllib.error.URLError) as e:
    logging.info("Could not load url=%s", url)
    raise e


def update_url_query(url: str, query_params: Dict[str, str]) -> str:
  parsed_url = urlparse.urlparse(url)
  query = dict(urlparse.parse_qsl(parsed_url.query))
  query.update(query_params)
  parsed_url = parsed_url._replace(query=urlparse.urlencode(query, doseq=True))
  return parsed_url.geturl()
