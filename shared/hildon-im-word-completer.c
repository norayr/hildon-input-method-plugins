/**
   @file hildon-im-word-completer.c

   This file may contain parts derived by disassembling of binaries under
   Nokia's copyright, see http://tablets-dev.nokia.com/maemo-dev-env-downloads.php

   The original licensing conditions apply to all those derived parts as well
   and you accept those by using this file.
*/

#include <glib.h>
#include <errno.h>
#include <string.h>

#include <gtk/gtkwidget.h>
#include <gtk/gtkcontainer.h>

#include <hildon-im-ui.h>
#include <imengines-wp.h>

#include "hildon-im-word-completer.h"

static HildonIMWordCompleter *hildon_im_word_completer = NULL;

enum{
  HILDON_IM_WORD_COMPLETER_PROP_LANGUAGE = 1,
  HILDON_IM_WORD_COMPLETER_PROP_SECOND_LANGUAGE,
  HILDON_IM_WORD_COMPLETER_PROP_DUAL_DICTIONARY,
  HILDON_IM_WORD_COMPLETER_PROP_MAX_CANDIDATES,
  HILDON_IM_WORD_COMPLETER_PROP_MAX_SUFFIX,
  HILDON_IM_WORD_COMPLETER_PROP_LAST
};

struct _HildonIMWordCompleterPrivate{
  gchar *lang[2];
  gboolean dual_dictionary;
  gint max_candidates;
  glong max_suffix;
  gchar *base_dir;
};

#define HILDON_IM_WORD_COMPLETER_GET_PRIVATE(completer) \
  ((HildonIMWordCompleterPrivate *)hildon_im_word_completer_get_instance_private(completer))

typedef struct _HildonIMWordCompleterPrivate HildonIMWordCompleterPrivate;

G_DEFINE_TYPE_WITH_PRIVATE(
    HildonIMWordCompleter, hildon_im_word_completer, G_TYPE_OBJECT
);

gpointer
hildon_im_word_completer_new()
{
  return g_object_new(HILDON_IM_WORD_COMPLETER_TYPE, NULL);
}

static void
hildon_im_word_completer_init(HildonIMWordCompleter *wc)
{
  HildonIMWordCompleterPrivate *priv = HILDON_IM_WORD_COMPLETER_GET_PRIVATE(wc);

  priv->lang[0] = g_strdup("en_GB");
  priv->lang[1] = g_strdup("en_GB");
  priv->dual_dictionary = FALSE;
  priv->max_candidates = 1;

  imengines_wp_init("ezitext");

  priv->base_dir =
      g_build_filename(g_get_home_dir(), ".osso/dictionaries", NULL);

  if (g_mkdir_with_parents(priv->base_dir, 0755))
  {
    g_warning("Couldn't create directory %s: %s", priv->base_dir,
              strerror(errno));
  }
  else
    imengines_wp_set_data("base-dir", priv->base_dir);

  imengines_wp_set_data("ezitext", (void *)0xBBC58F26); /* WTF ?!? */
  imengines_wp_set_prediction_language(priv->lang[0], 0);
  imengines_wp_set_prediction_language(priv->lang[0], 1);
  imengines_wp_set_max_candidates(priv->max_candidates);
  imengines_wp_attach_dictionary(1, 1);
  imengines_wp_attach_dictionary(2, 1);
  imengines_wp_attach_dictionary(0, 1);
}

static GObject *
hildon_im_word_completer_constructor(GType gtype, guint n_properties,
                                     GObjectConstructParam *properties)
{
  GObject *obj;

  if (hildon_im_word_completer)
    obj = g_object_ref(G_OBJECT(hildon_im_word_completer));
  else
  {
    obj = G_OBJECT_CLASS(hildon_im_word_completer_parent_class)->
        constructor(gtype, n_properties, properties);
    hildon_im_word_completer = HILDON_IM_WORD_COMPLETER(obj);
  }

  return obj;
}

static void hildon_im_word_completer_finalize(GObject *object)
{
  HildonIMWordCompleter *wc;
  HildonIMWordCompleterPrivate *priv;

  wc = HILDON_IM_WORD_COMPLETER(object);
  priv = HILDON_IM_WORD_COMPLETER_GET_PRIVATE (wc);

  hildon_im_word_completer_save_data(wc);

  imengines_wp_detach_dictionary(2);
  imengines_wp_detach_dictionary(1);
  imengines_wp_detach_dictionary(0);
  imengines_wp_destroy();

  g_free(priv->base_dir);

  if (G_OBJECT_CLASS(hildon_im_word_completer_parent_class)->finalize)
    G_OBJECT_CLASS(hildon_im_word_completer_parent_class)->finalize(object);

  hildon_im_word_completer = 0;
}

static void hildon_im_word_completer_set_property(GObject *object,
                                      guint prop_id,
                                      const GValue *value,
                                      GParamSpec *pspec)
{
  HildonIMWordCompleterPrivate *priv;

  g_return_if_fail(HILDON_IM_IS_WORD_COMPLETER(object));

  priv = HILDON_IM_WORD_COMPLETER_GET_PRIVATE(HILDON_IM_WORD_COMPLETER(object));

  switch (prop_id)
  {
    case HILDON_IM_WORD_COMPLETER_PROP_LANGUAGE:
      g_free(priv->lang[0]);
      priv->lang[0] = g_value_dup_string(value);

      if(priv->lang[0] && *priv->lang[0])
      {
        imengines_wp_attach_dictionary(0,1);
        imengines_wp_set_prediction_language(priv->lang[0], 0);

        if(priv->dual_dictionary)
          imengines_wp_set_prediction_language(priv->lang[0], 1);
        else
        {
          g_free(priv->lang[1]);
          priv->lang[1] = g_strdup(priv->lang[0]);
          imengines_wp_set_prediction_language(priv->lang[1], 1);
        }
      }
      else
        imengines_wp_detach_dictionary(0);
      break;
    case HILDON_IM_WORD_COMPLETER_PROP_SECOND_LANGUAGE:
      g_free(priv->lang[1]);
      priv->lang[1] = g_value_dup_string(value);

      if(priv->lang[1] && *priv->lang[1])
      {
        if(!priv->dual_dictionary)
        {
          if(!priv->lang[0] || !*priv->lang[0])
            break;
          imengines_wp_attach_dictionary(0,1);
          imengines_wp_set_prediction_language(priv->lang[0], 1);
        }
        else
        {
          imengines_wp_attach_dictionary(0,1);
          imengines_wp_set_prediction_language(priv->lang[1], 1);
        }
      }
      else
      {
        if(!priv->lang[0] || !*priv->lang[0])
          imengines_wp_detach_dictionary(0);
        else
        {
          imengines_wp_attach_dictionary(0,1);
          imengines_wp_set_prediction_language(priv->lang[0], 0);
        }
      }
      break;
    case HILDON_IM_WORD_COMPLETER_PROP_DUAL_DICTIONARY:
    {
      gboolean dual_dictionary = g_value_get_boolean(value);

      if(priv->dual_dictionary == dual_dictionary)
        break;

      priv->dual_dictionary = dual_dictionary;

      /* hmm, what if second dictionary is not set? */
      if(dual_dictionary)
        imengines_wp_set_prediction_language(priv->lang[1], 1);
      else
        imengines_wp_set_prediction_language(priv->lang[0], 1);
      break;
    }
    case HILDON_IM_WORD_COMPLETER_PROP_MAX_CANDIDATES:
    {
      gint max_candidates = g_value_get_int(value);

      if(priv->max_candidates == max_candidates)
        break;

      priv->max_candidates = max_candidates;
      imengines_wp_set_max_candidates(max_candidates);

      break;
    }
    case HILDON_IM_WORD_COMPLETER_PROP_MAX_SUFFIX:
      priv->max_suffix = g_value_get_long(value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
      break;
  }
}

static void
hildon_im_word_completer_get_property(GObject *object,
                                      guint prop_id,
                                      GValue *value,
                                      GParamSpec *pspec)
{
  HildonIMWordCompleterPrivate *priv;

  g_return_if_fail(HILDON_IM_IS_WORD_COMPLETER(object));

  priv = HILDON_IM_WORD_COMPLETER_GET_PRIVATE(HILDON_IM_WORD_COMPLETER(object));

  switch (prop_id)
  {
    case HILDON_IM_WORD_COMPLETER_PROP_LANGUAGE:
      g_value_set_string(value, priv->lang[0]);
      break;
    case HILDON_IM_WORD_COMPLETER_PROP_SECOND_LANGUAGE:
      g_value_set_string(value, priv->lang[1]);
      break;
    case HILDON_IM_WORD_COMPLETER_PROP_DUAL_DICTIONARY:
      g_value_set_boolean(value, priv->dual_dictionary);
      break;
    case HILDON_IM_WORD_COMPLETER_PROP_MAX_CANDIDATES:
      g_value_set_int(value, priv->max_candidates);
      break;
    case HILDON_IM_WORD_COMPLETER_PROP_MAX_SUFFIX:
      g_value_set_long(value, priv->max_suffix);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
      break;
  }
}

static void
hildon_im_word_completer_class_init(HildonIMWordCompleterClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS(klass);

  object_class->constructor = hildon_im_word_completer_constructor;
  object_class->set_property = hildon_im_word_completer_set_property;
  object_class->get_property = hildon_im_word_completer_get_property;
  object_class->finalize = hildon_im_word_completer_finalize;

  g_object_class_install_property(
        object_class, HILDON_IM_WORD_COMPLETER_PROP_LANGUAGE,
        g_param_spec_string(
          "language",
          "Language",
          "The language used in word completion",
          "en_GB",
          G_PARAM_WRITABLE | G_PARAM_READABLE));

  g_object_class_install_property(
        object_class, HILDON_IM_WORD_COMPLETER_PROP_SECOND_LANGUAGE,
        g_param_spec_string(
          "second-language",
          "Second Language",
          "The second language used in word completion",
          "en_GB",
          G_PARAM_WRITABLE | G_PARAM_READABLE));

  g_object_class_install_property(
        object_class, HILDON_IM_WORD_COMPLETER_PROP_DUAL_DICTIONARY,
        g_param_spec_boolean(
          "dual-dictionary",
          "Dual Dictionary",
          "Dual dictionary used in word completion",
          FALSE,
          G_PARAM_WRITABLE | G_PARAM_READABLE));

  g_object_class_install_property(
        object_class, HILDON_IM_WORD_COMPLETER_PROP_MAX_CANDIDATES,
        g_param_spec_int(
          "max_candidates",
          "Max. candidates",
          "The max. number of candidates for word completion",
          0,
          G_MAXINT,
          1,
          G_PARAM_WRITABLE|G_PARAM_READABLE));

   g_object_class_install_property(
         object_class, HILDON_IM_WORD_COMPLETER_PROP_MAX_SUFFIX,
         g_param_spec_long(
           "min_candidate_suffix_length",
           "Min. candidate length",
           "The minimum length of the suffix of the selected candidate.",
           0,
           G_MAXINT,
           2,
           G_PARAM_WRITABLE|G_PARAM_READABLE));
}

void
hildon_im_word_completer_save_data(HildonIMWordCompleter *wc)
{
  imengines_wp_save_dictionary(1);
  imengines_wp_save_dictionary(2);
}

void
hildon_im_word_completer_configure(HildonIMWordCompleter *wc, HildonIMUI *ui)
{
  guint lang_index;
  gchar *key;
  GConfValue *value;
  const gchar* first_lang;
  const gchar *second_lang;
  const gchar *lang[2];

  g_return_if_fail(ui != NULL && wc != NULL);

  lang[0] = hildon_im_ui_get_language_setting(ui, 0);
  lang[1] = hildon_im_ui_get_language_setting(ui, 1);
  lang_index = hildon_im_ui_get_active_language_index(ui);

  first_lang = lang[lang_index];
  second_lang = lang[lang_index? 0 : 1]; /* not a bug :) */

  if (first_lang && *first_lang)
  {
    key = g_strdup_printf(
          HILDON_IM_GCONF_LANG_DIR "/%s/dictionary", first_lang);
    value = gconf_client_get(ui->client, key, NULL);

    if (value)
    {
      g_object_set(wc, "language", gconf_value_get_string(value), NULL);
      gconf_value_free(value);
    }
    else
      g_object_set(wc, "language", "", NULL);

    g_free(key);
  }
  else
    g_object_set(wc, "language", "", NULL);

  if (second_lang && *second_lang)
  {
    key = g_strdup_printf(
          HILDON_IM_GCONF_LANG_DIR "/%s/dictionary",second_lang);
    value = gconf_client_get(ui->client, key, NULL);

    if (value)
    {
      g_object_set(wc, "second-language", gconf_value_get_string(value), NULL);
      gconf_value_free(value);
    }
    else
      g_object_set(wc, "second-language", "", NULL);

    g_free(key);
  }
  else
    g_object_set(wc, "second-language", "", NULL);

  value = gconf_client_get(ui->client,
                           HILDON_IM_GCONF_DIR "/dual-dictionary", NULL);

  if (value)
  {
    g_object_set(wc, "dual-dictionary", gconf_value_get_bool(value), NULL);
    gconf_value_free(value);
  }
}

static const gchar *_special_chars[]={".",",",":",";","!","?"};

static gboolean
hildon_im_word_completer_word_at_index(guint dict, gchar *word, guint index)
{
  gboolean result;
  guint idx;

  result = imengines_wp_word_exists(word, dict, &idx);

  if (result)
    result = (idx == index);

  return result;
}

static
gboolean hildon_im_word_completer_move_word(int dict, gchar *word, guint index)
{
  imengines_wp_delete_word(word, dict, index);

  return (imengines_wp_add_word(word, dict, index) == 0);
}

gboolean
hildon_im_word_completer_hit_word(HildonIMWordCompleter *wc, const gchar *text,
                                  gboolean b)
{
  gboolean ret = FALSE;
  gchar *word;
  HildonIMWordCompleterPrivate *priv;
  gchar *p;
  gunichar last_char;
  const gchar *special_chars[6];
  const gchar* str;
  int i;

  priv = HILDON_IM_WORD_COMPLETER_GET_PRIVATE (wc);

  str = text;

  if (!str || !*str || !g_utf8_validate(str, -1, 0))
    return FALSE;

  while (1)
  {
    gunichar uc = g_utf8_get_char_validated(str, -1);

    if (((int)uc) <= -1)
      break;

    if (!g_unichar_isalnum(uc) &&
        !g_unichar_ismark(uc) &&
        (*str != '-') && (*str != '_') && (*str != '\'') && (*str != '&'))
    {
      break;
    }

    str = g_utf8_next_char(str);

    if (!*str)
      goto go_on;
  }

  if (g_utf8_next_char(str))
    return FALSE;

go_on:

  word = g_utf8_strdown(text, -1);
  memcpy(special_chars, _special_chars, sizeof(special_chars));

  p = g_utf8_offset_to_pointer(word, g_utf8_strlen(word, -1) - 1);
  last_char = g_utf8_get_char(p);

  for (i = 0; i < G_N_ELEMENTS(special_chars); i++)
  {
    gchar *s = g_utf8_normalize(special_chars[i], -1, G_NORMALIZE_DEFAULT);
    gunichar uc = g_utf8_get_char_validated(s, -1);
    g_free(s);

    if (uc == last_char)
    {
      *p = 0;
      break;
    }
  }

  for (i = 0; (i < (priv->dual_dictionary ? 2 : 1)) && !ret; i++)
  {
    gboolean has_lang = *priv->lang[0] || *priv->lang[1];
    gboolean move_word = FALSE;

    if (b)
    {
      if (hildon_im_word_completer_word_at_index(1, word, i))
        ret = hildon_im_word_completer_move_word(1, word, i);
      else
      {
        if (hildon_im_word_completer_word_at_index(2, word, i))
          move_word = TRUE;
        else if (has_lang && hildon_im_word_completer_word_at_index(0, word, i))
          ret = TRUE;
      }
    }
    else
    {
      if (has_lang && hildon_im_word_completer_word_at_index(0, word, i))
        move_word = TRUE;
      else
      {
        if (!hildon_im_word_completer_word_at_index(1, word, i))
        {
          if (hildon_im_word_completer_word_at_index(2, word, i))
            move_word = TRUE;
        }
        else
        {
          if (hildon_im_word_completer_word_at_index(2, word, i))
            ret = hildon_im_word_completer_move_word(2, word, i);

          imengines_wp_delete_word(word, 1, i);
        }
      }
    }

    if (move_word)
      ret = hildon_im_word_completer_move_word(2, word, i);
  }

  if (!ret)
    ret = hildon_im_word_completer_move_word(1, word, 0);

  g_free(word);

  return ret;
}

gchar *
hildon_im_word_completer_get_predicted_suffix(HildonIMWordCompleter *wc,
                                              gchar *previous_word,
                                              const gchar *current_word,
                                              gchar **out)
{
  gchar *candidate = hildon_im_word_completer_get_one_candidate(
        wc, previous_word, current_word);

  if (current_word && *current_word && candidate)
  {
    gchar *rv;
    size_t len = strlen(current_word);
    size_t clen = strlen(candidate);

    if (len < clen)
      rv = g_strdup(&candidate[len]);
    else
      rv = g_strdup("");

    if (out && len < clen)
      *out = g_strdup(candidate);

    g_free(candidate);
    return rv;
  }

  return g_strdup("");
}

static gboolean
is_uppercase(const gchar *s)
{
  gboolean rv = FALSE;

  if (!s)
    return FALSE;

  while (s)
  {
    rv = g_unichar_isupper(g_utf8_get_char(s));
    s = g_utf8_next_char(s);

    if (!rv || !s)
      break;
  }

  return rv;
}

gchar *
hildon_im_word_completer_get_one_candidate(HildonIMWordCompleter *wc,
                                          const gchar *previous_word,
                                          const gchar *current_word)
{
  HildonIMWordCompleterPrivate *priv =
      HILDON_IM_WORD_COMPLETER_GET_PRIVATE (wc);
  glong len = g_utf8_strlen(current_word, -1);
  imengines_wp_candidates candidates={0,};
  gchar *prev = NULL;
  gchar *curr = NULL;
  gchar *rv = NULL;

  if (previous_word)
    prev = g_utf8_strdown(previous_word, -1);

  if (current_word)
    curr = g_utf8_strdown(current_word, -1);

  if (!imengines_wp_get_candidates(prev, curr, &candidates))
  {
    int i;

    for (i = 0; i < candidates.number_of_candidates; i++)
    {
      const gchar *c = candidates.candidate[i];

      if (priv->max_suffix < g_utf8_strlen(c, -1) - len)
      {
        if (is_uppercase(current_word) && g_utf8_strlen(current_word, -1) > 1)
          rv = g_utf8_strup(c, -1);
        else
          rv = g_strdup(c);

        if (rv)
          break;
      }
    }
  }

  g_free(prev);
  g_free(curr);

  return rv;
}

gboolean
hildon_im_word_completer_is_interesting_key(HildonIMWordCompleter *wc,
                                            const gchar *key)
{
  if (!g_strcmp0(key, HILDON_IM_GCONF_DIR "/dual-dictionary") ||
      !g_strcmp0(key, HILDON_IM_GCONF_LANG_DIR "/current"))
  {
    return TRUE;
  }

  return g_str_has_prefix(key, HILDON_IM_GCONF_LANG_DIR) &&
      g_str_has_suffix(key, "dictionary");
}

void
hildon_im_word_completer_remove_word(HildonIMWordCompleter *wc,
                                     const gchar *word)
{
  guint idx = 0;

  if (imengines_wp_word_exists(word, 1, &idx))
    imengines_wp_delete_word(word, 1, idx);
}

gboolean
hildon_im_word_completer_add_to_dictionary(HildonIMWordCompleter *wc,
                                           const gchar *word)
{
  guint idx;

  if (imengines_wp_word_exists(word, 1, &idx))
    imengines_wp_delete_word(word, 1, idx);
  else if (!imengines_wp_word_exists(word, 0, &idx))
    return TRUE;

  imengines_wp_add_word(word, 2, idx);

  return TRUE;
}

gchar **
hildon_im_word_completer_get_candidates(HildonIMWordCompleter *wc,
                                        const gchar *previous_word,
                                        const gchar *current_word)
{
  gchar **rv = NULL;
  gchar *curr = NULL;
  gchar *prev = NULL;
  imengines_wp_candidates candidates={};

  if (previous_word)
    prev = g_utf8_strdown(previous_word, -1);

  if (current_word)
    curr = g_utf8_strdown(current_word, -1);

  if (!imengines_wp_get_candidates(prev, curr, &candidates) ||
      !candidates.number_of_candidates)
  {
    int i;

    rv = g_new0(gchar *, candidates.number_of_candidates + 1);

    for (i = 0; i < candidates.number_of_candidates; i++)
    {
      if (is_uppercase(current_word) &&
          g_utf8_strlen(current_word, -1) > 1)
      {
        rv[i] = g_strdup(candidates.candidate[i - 1]);
      }
      else
        rv[i] = g_utf8_strup(candidates.candidate[i], -1);
    }
  }

  g_free(prev);
  g_free(curr);

  return rv;
}
