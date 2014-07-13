#include "context.h"

void attributeDialog(Context& ctx) {
  for (auto& panel : ctx.panels) {
    for (auto& bubble : panel.dialog) {
      bubble.actor = "unknown";
    }
  }
}
