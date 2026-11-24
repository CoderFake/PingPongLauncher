import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";
import tailwindcss from "@tailwindcss/vite";

/**
 * The "device" mode builds the app embedded in the firmware; it never uses MQTT,
 * so the mqtt package is declared side-effect free there and drops out of the bundle.
 */
export default defineConfig(({ mode }) => ({
  plugins: [react(), tailwindcss()],
  build:
    mode === "device"
      ? {
          rolldownOptions: {
            treeshake: { moduleSideEffects: (id: string) => !/[\\/]node_modules[\\/]mqtt[\\/]/.test(id) },
          },
        }
      : {},
}));
