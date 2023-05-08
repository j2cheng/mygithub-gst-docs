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

// #include <gst/gst.h>
// #include <string.h>
// #include <stdio.h>
// #include <locale.h>
// #include <stdbool.h>
#include "gst_element_print_properties.h"
#include <android/log.h>

void       print_column_titles( guint c2w, guint c3w, guint c4w );
void       print_element_info( GstElement *element, guint c2w, guint c3w, guint c4w  );
gchar *    flags_to_string( GFlagsValue * vals, guint flags );
void       print_caps( const GstCaps * caps, const gchar * pfx );
gboolean   print_field( GQuark field, const GValue * value, gpointer pfx );

void gst_element_print_properties( GstElement * element )
{
   /////////////////////////////////////////////////////////////////////////////
   //
   // Formatting setup
   //
   //    Change the valuses of c2w, c3w and c4w to adjust the 2nd, 3rd and 4th
   //    column widths, respectively.  The gutter width is fixed at 3 and
   //    alwasys prints as " | ".  Column 1 has a fixed width of 3.
   //
   //    The first two rows for each element's output are its element class
   //    name (e.g. "GstAudioResample") and its element factory name
   //    ("audioresample").  The long element factory name ("Audio resampler")
   //    is in column 4 following the element factory name.
   //
   //    Most properties use this format.  Multivalued items like CAPS, certain
   //    GST_TYPEs and enums are different.
   //
   //      Column 1  contains the rwc, "readable", "writable", "controllable"
   //                flags of the property.
   //      Column 2  contains the property name
   //      Column 3  contains the current value
   //      Column 4  contains the property type, e.g. G_TYPE_INT
   //      Column 5  contains the range, if there is one, and the default.
   //                The range is encosed in parentheses. e.g.  "(1-10)   5"
   //
   //    CAPS, enums, flags and some undefined items have no columns 4 or 5 and
   //    column 3 will contain a description of the item.  Additional rows may
   //    list specific valused (CAPS and flags).
   //
   //    String values are enclosed in double quotes.  A missing right quote
   //    inidicates the string had been truncated.
   //
   //  Screen column
   //  ----+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9--->
   //
   //  formatted columns with built in gutters
   //  --- | ---------c2---------- | ---------c3-------- | -----------c4---------- | --> unspecified
   //
   //  <-->|<--- property name --->|<-- current value -->|<-------- type --------->|<----- range and default ----->
   //      | ELEMENT CLASS NAME    | GstAudioResample    |                         |
   //      | ELEMENT FACTORY NAME  | audioresample       | Audio resampler         |
   //  RW- | name                  | "audioResampler"    | G_TYPE_STRING           | null
   //  RW- | qos                   | false               | G_TYPE_BOOLEAN          | false
   //  RW- | quality               | 8                   | G_TYPE_INT              | (0 - 10)   4
   //
   /////////////////////////////////////////////////////////////////////////////

   const guint    c2w = 21;   // column 2 width
   const guint    c3w = 19;   // column 3 width
   const guint    c4w = 23;   // column 4 width

   /////////////////////////////////////////////////////////////////////////////
   // end configuration variables.
   /////////////////////////////////////////////////////////////////////////////

   GParamSpec  **property_specs;
   guint       num_properties, i;
   gboolean    readable;
   gboolean    first_flag;


   g_return_if_fail( element != NULL );

   property_specs = g_object_class_list_properties(
                                      G_OBJECT_GET_CLASS( element ),
                                      &num_properties );

   /*--- draw the header information ---*/
   print_column_titles( c2w, c3w, c4w );
   print_element_info( element, c2w, c3w, c4w );


   for ( i = 0 ; i < num_properties ; i++ )
   {
      GValue      value = { 0, };
      GParamSpec  *param = property_specs[ i ];

      readable   = FALSE;
      first_flag = TRUE;

      g_value_init( &value, param->value_type );

      gchar flags[4];
      flags[0] = '-';
      flags[1] = '-';
      flags[2] = '-';
      flags[3] = 0x0;

      if ( param->flags & G_PARAM_READABLE )
      {
         g_object_get_property( G_OBJECT( element ), param->name, &value );
         readable = TRUE;
         flags[0] = 'r';
      }

      if ( param->flags & G_PARAM_WRITABLE )
         flags[1] = 'w';

      if ( param->flags & GST_PARAM_CONTROLLABLE )
         flags[2] = 'c';

      __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", "%s |", flags );
      __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", " %-*s | ", c2w, g_param_spec_get_name( param ));

      switch( G_VALUE_TYPE( &value ))
      {
         case G_TYPE_STRING:  // String
         {
            GParamSpecString *pstring = G_PARAM_SPEC_STRING( param );
            if ( readable )                                        /* current */
            {
               const char  *string_val = g_value_get_string( &value );
               gchar       work_string[100];

               if ( string_val == NULL )
                  sprintf( work_string, "\"%s\"","null" );
               else
                  sprintf( work_string, "\"%s\"", string_val );
               __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", "%-*.*s", c3w, c3w, work_string );
            }
            else
            {
               __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", "%-*s", c3w, "<not readable>" );       /* alt current */
            }
            __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", " | %-*s", c4w, "G_TYPE_STRING" );               /* type */

            if ( pstring->default_value == NULL )
               __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", " | %s", "null" );                         /* default */
            else
               __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", " | \"%s\"", pstring->default_value );     /* default */
            break;
         }

         case G_TYPE_BOOLEAN: //  Boolean
         {
            GParamSpecBoolean *pboolean = G_PARAM_SPEC_BOOLEAN( param );
            if ( readable )                                        /* current */
               __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", "%-*s", c3w,
                        ( g_value_get_boolean( &value ) ? "true" : "false" ));
            else
               __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", "%-*s", c3w, "<not readable>" );
            __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", " | %-*s", c4w, "G_TYPE_BOOLEAN" );              /* type */
            __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", " | %s ",                                     /* default */
                     (pboolean->default_value ? "true" : "false"));
            break;
         }

         case G_TYPE_ULONG:  //  Unsigned Long
         {
            GParamSpecULong *pulong = G_PARAM_SPEC_ULONG( param );
            if ( readable )                                        /* current */
               __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", "%-*lu", c3w, g_value_get_ulong( &value ));
            else
               __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", "%-*s", c3w, "<not readable>" );
            __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", " | %-*s", c4w, "G_TYPE_ULONG" );                /* type */
            __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", " | (%lu - %lu)   %lu ",
                     pulong->minimum, pulong->maximum,               /* range */
                     pulong->default_value );                      /* default */
            break;
         }

         case G_TYPE_LONG:  //  Long
         {
            GParamSpecLong *plong = G_PARAM_SPEC_LONG( param );
            if ( readable )                                        /* current */
               __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", "%-*ld", c3w, g_value_get_long( &value ));
            else
               __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", "%-*s", c3w, "<not readable>" );
            __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", " | %-*s", c4w, "G_TYPE_LONG" );                 /* type */
            __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", " | (%ld - %ld)   %ld ",
                     plong->minimum, plong->maximum,                 /* range */
                     plong->default_value );                       /* default */
            break;
         }

         case G_TYPE_UINT:  //  Unsigned Integer
         {
            GParamSpecUInt *puint = G_PARAM_SPEC_UINT( param );
            if ( readable )                                        /* current */
               __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", "%-*u", c3w, g_value_get_uint( &value ));
            else
               __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", "%-*s", c3w, "<not readable>" );
            __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", " | %-*s", c4w, "G_TYPE_UINT" );                 /* type */
            __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", " | (%u - %u)   %u ",
                     puint->minimum, puint->maximum,                 /* range */
                     puint->default_value );                       /* default */
            break;
         }

         case G_TYPE_INT:  //  Integer
         {
            GParamSpecInt *pint = G_PARAM_SPEC_INT( param );
            if ( readable )                                        /* current */
               __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", "%-*d", c3w, g_value_get_int( &value ));
            else
               __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", "%-*s", c3w, "<not readable>" );
            __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", " | %-*s", c4w, "G_TYPE_INT" );                  /* type */
            __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", " | (%d - %d)   %d ",
                     pint->minimum, pint->maximum,                   /* range */
                     pint->default_value );                        /* default */
            break;
         }

         case G_TYPE_UINT64: //  Unsigned Integer64.
         {
            GParamSpecUInt64 *puint64 = G_PARAM_SPEC_UINT64( param );
            if ( readable )                                        /* current */
               __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", "%-*" G_GUINT64_FORMAT,
                        c3w, g_value_get_uint64( &value ));
            else
               __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", "%-*s", c3w, "<not readable>" );
            __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", " | %-*s", c4w, "G_TYPE_UINT64" );               /* type */
            __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", " | (%" G_GUINT64_FORMAT " - %" G_GUINT64_FORMAT ")"
                     "   %" G_GUINT64_FORMAT " ",
                     puint64->minimum, puint64->maximum,             /* range */
                     puint64->default_value );                     /* default */
            break;
         }

         case G_TYPE_INT64: // Integer64
         {
            GParamSpecInt64 *pint64 = G_PARAM_SPEC_INT64( param );
            if ( readable )                                        /* current */
               __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", "%-*" G_GINT64_FORMAT, c3w, g_value_get_int64( &value ));
            else
               __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", "%-*s", c3w, "<not readable>" );
            __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", " | %-*s", c4w, "G_TYPE_INT64"  );               /* type */
            __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", " | (%" G_GINT64_FORMAT " - %" G_GINT64_FORMAT ")"
                     "   %" G_GINT64_FORMAT " ",
                     pint64->minimum, pint64->maximum,               /* range */
                     pint64->default_value );                      /* default */
            break;
         }

         case G_TYPE_FLOAT:  //  Float.
         {
            GParamSpecFloat *pfloat = G_PARAM_SPEC_FLOAT( param );
            if ( readable )                                        /* current */
               __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", "%-*g", c3w, g_value_get_float( &value ));
            else
               __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", "%-*s", c3w, "<not readable>" );
            __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", " | %-*s", c4w, "G_TYPE_FLOAT" );                /* type */
            __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", " | (%g - %g)   %g ",
                      pfloat->minimum, pfloat->maximum,              /* range */
                      pfloat->default_value );                     /* default */
            break;
         }

         case G_TYPE_DOUBLE:  //  Double
         {
            GParamSpecDouble *pdouble = G_PARAM_SPEC_DOUBLE( param );
            if ( readable )                                        /* current */
               __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", "%-*g", c3w, g_value_get_double( &value ));
            else
               __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", "%-*s", c3w, "<not readable>" );
            __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", " | %-*s", c4w, "G_TYPE_DOUBLE" );               /* type */
            __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", " | (%g - %g)   %g ",
                     pdouble->minimum, pdouble->maximum,             /* range */
                     pdouble->default_value );                     /* default */
            break;
         }

         default:
         if ( param->value_type == GST_TYPE_CAPS )
         {
            const GstCaps *caps = gst_value_get_caps( &value );
            if ( !caps )
               __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", "%-*s | %-*.*s |",
                        c3w, "Caps (NULL)",
                        c4w, c4w, " " );
            else
            {
               gchar   prefix_string[ 100 ];
               sprintf( prefix_string, "    | %-*.*s | ", c2w, c2w, " " );
               print_caps( caps, prefix_string );
            }
         }

         else if ( G_IS_PARAM_SPEC_ENUM( param ))
         {
            GParamSpecEnum *penum = G_PARAM_SPEC_ENUM( param );
            GEnumValue     *values;
            guint          j = 0;
            gint           enum_value;
            const gchar    *def_val_nick = "", *cur_val_nick = "";
            gchar          work_string[100];

            values = G_ENUM_CLASS( g_type_class_ref(param->value_type))->values;
            enum_value = g_value_get_enum( &value );

            while ( values[ j ].value_name )
               {
               if ( values[ j ].value == enum_value )
                  cur_val_nick = values[ j ].value_nick;
               if ( values[ j ].value == penum->default_value )
                  def_val_nick = values[ j ].value_nick;
               j++;
               }

            sprintf( work_string, "%d, \"%s\"", enum_value, cur_val_nick );
            __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", "%-*.*s", c3w, c3w, work_string );
            __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", " | Enum \"%s\" : %d, \"%s\"",
                     g_type_name( G_VALUE_TYPE( &value )),
                     penum->default_value, def_val_nick );
         }

         else if ( G_IS_PARAM_SPEC_FLAGS( param ))
         {
            GParamSpecFlags   *pflags = G_PARAM_SPEC_FLAGS( param );
            GFlagsValue       *vals;
            gchar             *cur, *def;
            gchar             work_string[512];

            vals = pflags->flags_class->values;
            cur = flags_to_string( vals,
                                   g_value_get_flags( &value ));   /* current */
            def = flags_to_string( vals, pflags->default_value );  /* default */

            /* current */
            sprintf( work_string, "0x%08x, \"%s\"",
                     g_value_get_flags( &value ), cur  );
            __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", "%-*.*s", c3w, c3w, work_string );

            /* type */
            sprintf( work_string, "Flags \"%s\"",
                     g_type_name( G_VALUE_TYPE( &value )) );
            __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", "%-*.*s", c4w, c4w, work_string );

            /* default */
            __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", " | 0x%08x, \"%s\"",
                     pflags->default_value, def );

            /* values list */
            while ( vals[0].value_name )
            {
               sprintf( work_string, "\n    | %-*.*s |   (0x%08x): %-16s - %s",
                        c2w, c2w, "",
                        vals[0].value, vals[0].value_nick, vals[0].value_name );
               __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", "%s", work_string );
               ++vals;
            }

            g_free( cur );
            g_free( def );
         }

         else if ( G_IS_PARAM_SPEC_OBJECT( param ))
         {
            __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", "%-*.*s | Object of type \"%s\"",
                     c3w, c3w,
                     g_type_name( param->value_type ),
                     g_type_name( param->value_type ) );
         }

         else if ( G_IS_PARAM_SPEC_BOXED( param ))
         {
            __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", "%-*.*s | Boxed pointer of type \"%s\"",
                     c3w, c3w,
                     g_type_name( param->value_type ),
                     g_type_name( param->value_type ) );
         }

         else if ( G_IS_PARAM_SPEC_POINTER( param ))
         {
            if ( param->value_type != G_TYPE_POINTER )
            {
               __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", "%-*.*s | Pointer of type \"%s\"",
                        c3w, c3w,
                        g_type_name( param->value_type ),
                        g_type_name( param->value_type ) );
            }
            else
            {
               __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", "%-*.*s |", c3w, c3w, "Pointer." );
            }
         }

         else if ( param->value_type == G_TYPE_VALUE_ARRAY )
         {
            GParamSpecValueArray *pvarray = G_PARAM_SPEC_VALUE_ARRAY( param );
            if ( pvarray->element_spec )
            {
               __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", "%-*.*s :Array of GValues of type \"%s\"",
                        c3w, c3w,
                        g_type_name( pvarray->element_spec->value_type ),
                        g_type_name( pvarray->element_spec->value_type ) );
            }
            else
            {
               __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", "%-*.*s :", c3w, c3w, "Array of GValues" );
            }
         }

         else if ( GST_IS_PARAM_SPEC_FRACTION( param ))
         {
            GstParamSpecFraction   *pfraction = GST_PARAM_SPEC_FRACTION( param );
            gchar                  work_string[100];

            if ( readable )                                        /* current */
            {
               sprintf( work_string, "%d/%d",
                        gst_value_get_fraction_numerator( &value ),
                        gst_value_get_fraction_denominator( &value ));
               __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", "%-*.*s", c3w, c3w, work_string );
            }
            else
               __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", "%-*s", c3w, "<not readable>" );

            __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", " | %-*.*s",                                     /* type */
                     c3w, c3w,
                     " Fraction. " );
            __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", " | (%d/%d - %d/%d)",                           /* range */
                     pfraction->min_num, pfraction->min_den,
                     pfraction->max_num, pfraction->max_den );
            __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", "   %d/%d ",                                  /* default */
                     pfraction->def_num, pfraction->def_den );
         }

         else if ( G_IS_PARAM_SPEC_BOXED( param ))
         {
            __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", "%-*.*s | Boxed of type \"%s\"",
                     c3w, c3w,
                     g_type_name( param->value_type ),
                     g_type_name( param->value_type ) );
         }

         else
         {
            __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", "Unknown type %ld \"%s\"",
                     (glong)param->value_type,
                     g_type_name( param->value_type ));

         }
         break;
      }

      if ( !readable )
         __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", " Write only\n" );
      else
         __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", "\n" );

      g_value_reset( &value );
   }

   if ( 0 == num_properties )
      __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", "  none\n" );

   g_free( property_specs );
}

//------------------------------------------------------------------------------
void   print_column_titles( guint c2w, guint c3w, guint c4w )
   {
      //////////////////////////////////////////////////////////////////////////
      //
      // Create Header for property listing
      // RWF | --- element name ---- | ---------c3-------- | -----------c4---------- | --> unspecified
      //
      //////////////////////////////////////////////////////////////////////////
      gchar   work_string[200];
      gchar   dashes[] = "-----------------------------";
      gint    llen = 0;
      gint    rlen = 0;

      /*--- column 1 - RWC ---*/
      sprintf( work_string, "<-->|<" );

      /*--- column 2 - property name ---*/
      llen = (c2w - 15) / 2;               /* width of " property name " = 15 */
      rlen = c2w - 15 - llen;

      strncat( work_string, dashes, llen );
      strcat( work_string, " property name " );
      strncat( work_string, dashes, rlen );
      strcat( work_string, ">|<" );

      /*--- column 3 - current value ---*/
      llen = (c3w - 15) / 2;               /* width of " current value " = 15 */
      rlen = c3w - 15 - llen;

      strncat( work_string, dashes, llen );
      strcat( work_string, " current value " );
      strncat( work_string, dashes, rlen );
      strcat( work_string, ">|<" );

      /*--- column 4 - type ---*/
      llen = (c4w - 6) / 2;                          /* width of " type " = 6 */
      rlen = c4w - 6 - llen;

      strncat( work_string, dashes, llen );
      strcat( work_string, " type " );
      strncat( work_string, dashes, rlen );
      strcat( work_string, ">|<" );

      /*--- column 5 - range and default ---*/
      strcat( work_string, "----- range and default ----->" );

      __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print","\n%s\n", work_string );
   }

//------------------------------------------------------------------------------
void  print_element_info( GstElement *element, guint c2w, guint c3w, guint c4w  )
{
   /////////////////////////////////////////////////////////////////////////////
   //
   // Print element factory and class information as part of each header
   //
   /////////////////////////////////////////////////////////////////////////////
   gchar               work_string[100];
   GstElementFactory   *factory = gst_element_get_factory( element );

   sprintf( work_string, "ELEMENT CLASS NAME" );
   __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", "    | %-*s",   c2w, work_string );
   __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", " | %-*s",      c3w, g_type_name( G_OBJECT_TYPE( element )) );
   __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", " | %-*s | \n", c4w, "" );


   sprintf( work_string, "ELEMENT FACTORY NAME" );
   __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", "    | %-*s",   c2w, work_string );

   __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", " | %-*s",      c3w, gst_plugin_feature_get_name( GST_PLUGIN_FEATURE( factory ) ));
   __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", " | %-*s | \n", c4w,  gst_element_factory_get_longname( factory ));

// "Audio Resampler"   __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print", " | %-*s",      c3w, gst_element_factory_get_longname( gst_element_get_factory( element )) );


}

//------------------------------------------------------------------------------
gchar*   flags_to_string( GFlagsValue *vals, guint flags )
{
   /////////////////////////////////////////////////////////////////////////////
   //
   // List individual flags in separate rows
   //
   /////////////////////////////////////////////////////////////////////////////
    GString *s = NULL;
    guint flags_left, i;

    /* first look for an exact match and count the number of values */
    for ( i = 0; vals[i].value_name != NULL; ++i )
    {
       if ( vals[i].value == flags )
          return g_strdup( vals[i].value_nick );
    }

   s = g_string_new( NULL );

   /* we assume the values are sorted from lowest to highest value */
   flags_left = flags;
   while ( i > 0 )
   {
       --i;
       if (0 != vals[i].value && (flags_left & vals[i].value) == vals[i].value)
       {
          if ( 0 < s->len )
             g_string_append( s, " | " );
          g_string_append( s, vals[ i ].value_nick );
          flags_left -= vals[i].value;
          if ( 0 == flags_left )
             break;
       }
   }

   if ( 0 == s->len )
      g_string_assign( s, "(none)" );

   return g_string_free( s, FALSE );
}




/* Functions below print the Capabilities in a human-friendly format */
gboolean print_field(GQuark field, const GValue * value, gpointer pfx)
{
    gchar *str = gst_value_serialize(value);

    __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print","%s  %15s: %s\n", (gchar *) pfx, g_quark_to_string(field), str);
    g_free(str);
    return TRUE;
}

void print_caps(const GstCaps * caps, const gchar * pfx)
{
    guint i;

    g_return_if_fail(caps != NULL);

    if (gst_caps_is_any(caps))
    {
        __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print","%sANY\n", pfx);
        return;
    }
    if (gst_caps_is_empty(caps))
    {
        __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print","%sEMPTY\n", pfx);
        return;
    }

    for (i = 0; i < gst_caps_get_size(caps); i++)
    {
        GstStructure *structure = gst_caps_get_structure(caps, i);

        __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print","%d %s%s\n", i,pfx, gst_structure_get_name(structure));
        gst_structure_foreach(structure, print_field, (gpointer) pfx);
    }
}


/* Prints information about a Pad Template, including its Capabilities */
void print_pad_templates_information(GstElementFactory * factory)
{
    const GList *pads;
    GstStaticPadTemplate *padtemplate;

    __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print","Pad Templates for %s:\n",
            gst_element_factory_get_longname(factory));
    if (!gst_element_factory_get_num_pad_templates(factory))
    {
        __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print","  none\n");
        return;
    }

    pads = gst_element_factory_get_static_pad_templates (factory);
    while (pads)
    {
        padtemplate = (GstStaticPadTemplate *) (pads->data);
        pads = g_list_next(pads);

        if (padtemplate->direction == GST_PAD_SRC)
            __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print","  SRC template: '%s'\n", padtemplate->name_template);
        else if (padtemplate->direction == GST_PAD_SINK)
            __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print","  SINK template: '%s'\n", padtemplate->name_template);
        else
            __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print","  UNKNOWN!!! template: '%s'\n", padtemplate->name_template);

        if (padtemplate->presence == GST_PAD_ALWAYS)
            __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print","    Availability: Always\n");
        else if (padtemplate->presence == GST_PAD_SOMETIMES)
            __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print","    Availability: Sometimes\n");
        else if (padtemplate->presence == GST_PAD_REQUEST)
        {
            __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print","    Availability: On request\n");
        }
        else
            __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print","    Availability: UNKNOWN!!!\n");

        if (padtemplate->static_caps.string)
        {
            __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print","    Capabilities:\n");
            print_caps(gst_static_caps_get(&padtemplate->static_caps), "      ");
        }

        __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print","\n");
    }
}


void print_pad_capabilities(GstPad *pad)
{
    GstCaps *caps = NULL;

    /* Retrieve negotiated caps (or acceptable caps if negotiation is not finished yet) */
    caps = gst_pad_get_current_caps(pad);
    if (!caps)
    {
    	__android_log_print (ANDROID_LOG_ERROR,"gst_ele_print","Capabilities could not be obtained\n");
    }
    else
    {
    	__android_log_print (ANDROID_LOG_ERROR,"gst_ele_print","Printing negotiated caps\n");
        /* Print and free */
        __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print"," Caps:\n");
        print_caps(caps, "      ");
    }
    gst_caps_unref(caps);

}


/* Shows the CURRENT capabilities of the requested pad in the given element */
void print_pad_capabilities_from_element(GstElement *element, gchar *pad_name)
{
    GstPad *pad = NULL;

    /* Retrieve pad */
    pad = gst_element_get_static_pad(element, pad_name);
    if (!pad)
    {
        __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print","Could not retrieve pad '%s'\n", pad_name);
        return;
    }
    __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print","%s pad \t", pad_name);

    print_pad_capabilities(pad);

    gst_object_unref(pad);
}



void print_bin_elements(const char* message, GstBin* bin, int level)
{
    char* pszBinName;
    char* pszPadName;
    GstElement* pElement;
    GstPad* pPad;
    char indent[256];
    int i;
    gint refCount = -1;

    for(i=0; i<=level; i++)
    {
        indent[i] = '\t';
    }
    indent[i] = '\0';

    // get the name of the bin
    pszBinName = gst_element_get_name(bin);

    // get the ref count of the bin
    refCount = GST_OBJECT_REFCOUNT_VALUE(bin);

    // print information about the current element
    if(0 == level)
    {
        __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print","%s: %s state = %s, ref count = %d\n",
                message,
                pszBinName,
                gst_element_state_get_name(GST_STATE((GstElement*)bin)),
                refCount);
    }
    else
    {
        // The minus 1 on the refCount is due to the fact that this method
        //            has recursed into itself, and therefore, the element has had
        //            its reference incremented as a result of the
        //            gst_bin_iterate_elements() call.
        __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print","%s %s state = %s, ref count = %d\n",
                indent,
                pszBinName,
                gst_element_state_get_name(GST_STATE((GstElement*)bin)),
                refCount - 1);
    }


    // walk the pads and print out info...
    GstIterator* pItrSinkPads = gst_element_iterate_sink_pads((GstElement*)bin);
    while(GST_ITERATOR_OK == gst_iterator_next(pItrSinkPads, (GValue *)&pPad))
    {
        // ...print out the child elements
        // GORP: fill in

        pszPadName = gst_pad_get_name(pPad);
        __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print","%s   ## sinkpad = %s, blocked = %s, blocking = %s\n", indent,
                pszPadName, ((gst_pad_is_blocked(pPad) == 0)?"NO":"YES"),
                ((gst_pad_is_blocking(pPad) == 0)?"NO":"YES"));
        g_free(pszPadName);

        // the iterate function increments the refcount on the returned
        //            objects, so unref here
        gst_object_unref(pPad);
    }

    GstIterator* pItrSrcPads = gst_element_iterate_src_pads((GstElement*)bin);
    while(GST_ITERATOR_OK == gst_iterator_next(pItrSrcPads, (GValue *)&pPad))
    {
        // ...print out the child elements
        // GORP: fill in

        pszPadName = gst_pad_get_name(pPad);
        __android_log_print (ANDROID_LOG_ERROR,"gst_ele_print","%s   ## srcpad = %s, blocked = %s, blocking = %s\n", indent,
                pszPadName, ((gst_pad_is_blocked(pPad) == 0)?"NO":"YES"),
                ((gst_pad_is_blocking(pPad) == 0)?"NO":"YES"));
        g_free(pszPadName);

        // the iterate function increments the refcount on the returned
        //            objects, so unref here
        gst_object_unref(pPad);
    }



    // if we are processing a bin...
    if(TRUE == GST_IS_BIN(bin))
    {
        // ...then we need to recurse into it so
        // for each element...
        GstIterator* pItr1 = gst_bin_iterate_elements(bin);
        while(GST_ITERATOR_OK == gst_iterator_next(pItr1, (GValue *)&pElement))
        {
            // ...print out the child elements
            print_bin_elements(NULL, (GstBin*)pElement, level+1);

            // the iterate function increments the refcount on the returned
            //            objects, so unref here
            gst_object_unref(pElement);
        }

        // freeing the iterator should decrement the refcount on the bin itself
        gst_iterator_free(pItr1);
    }

    g_free (pszBinName);
}