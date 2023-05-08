/* GStreamer
 * Copyright (C) 2010 Wesley Miller <wmiller@sdr.com>
 *
 *
 *  gst_element_print_properties(): a tool to inspect GStreamer
 *                                  element properties
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef GST_ELEMENT_PRINT_PROPERTIES_H
#define GST_ELEMENT_PRINT_PROPERTIES_H
#include <gst/gst.h>

#ifdef __cplusplus
extern "C" {
#endif

void       gst_element_print_properties( GstElement * element );

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */
   

#endif
