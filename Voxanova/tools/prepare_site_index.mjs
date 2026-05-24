import { copyFileSync, existsSync } from "node:fs";
import { resolve } from "node:path";

const landingPath = resolve(process.cwd(), "site-dist", "landing.html");
const indexPath = resolve(process.cwd(), "site-dist", "index.html");

if (!existsSync(landingPath)) {
  throw new Error(`Missing built landing page at ${landingPath}`);
}

copyFileSync(landingPath, indexPath);
