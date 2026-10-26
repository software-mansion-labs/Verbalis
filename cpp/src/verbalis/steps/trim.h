#pragma once

#include <verbalis/text/document.h>

namespace verbalis::steps {

using text::Document;

/**
 * Normalizes whitespaces: removes them from both ends of the text and before punctuation (a , -> a,),
 * and collapses every other run into a single "\n" (if it contains a line break) or " ".
 */
void trim(Document& document);

} // namespace verbalis::steps
