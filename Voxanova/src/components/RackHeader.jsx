import { cls } from "../utils/format.js";

export function RackPowerButton({ active, onToggle, label, className }) {
  return (
    <button
      type="button"
      className={cls("rack-power-button", active && "active", className)}
      onClick={onToggle}
      aria-pressed={active}
      aria-label={`${active ? "Disable" : "Enable"} ${label}`}
    >
      {active ? "ON" : "OFF"}
    </button>
  );
}

export function RackTitleItem({ title, active, color = "blue", onToggle, label = title, className }) {
  return (
    <div className={cls("rack-title-item", active && "active", className)}>
      <RackPowerButton active={active} onToggle={onToggle} label={label} />
      <span className={cls("rack-title-text", `title-${color}`)}>{title}</span>
    </div>
  );
}
