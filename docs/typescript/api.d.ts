/** Ambient type declarations for the globals and built-in modules registered by QuickJS. */

type MonoPointer = number;
type MonoArgument = string | number | boolean;

declare class Vector3 {
  constructor();
  constructor(x: number, y: number, z: number);
  readonly x: number;
  readonly y: number;
  readonly z: number;
}

declare const console: {
  log(...values: unknown[]): void;
  warn(...values: unknown[]): void;
  error(...values: unknown[]): void;
};

declare const logger: {
  info(message: string): void;
};

declare const unity: {
  get_local_player(): MonoPointer;
  get_zone_system(): MonoPointer;
  get_env_man(): MonoPointer;
};

declare const mono: MonoApi;
interface MonoApi {
  get_root_domain(): MonoPointer;
  get_class(className: string, assembly: string): MonoPointer;
  get_method(className: string, methodName: string, parameterCount: number, assembly: string): MonoPointer;
  get_field(classPointer: MonoPointer, fieldName: string): MonoPointer;
  invoke_method(methodPointer: MonoPointer, objectPointer: MonoPointer, ...args: MonoArgument[]): MonoPointer;
  set_field_int(objectPointer: MonoPointer, fieldPointer: MonoPointer, value: number): void;
  set_field_float(objectPointer: MonoPointer, fieldPointer: MonoPointer, value: number): void;
  set_field_string(objectPointer: MonoPointer, fieldPointer: MonoPointer, value: string): void;
  get_field_value(objectPointer: MonoPointer, fieldPointer: MonoPointer): number;
  get_field_offset(fieldPointer: MonoPointer): number;
  get_static_field_value(className: string, fieldName: string): number;
  get_static_field_data(classPointer: MonoPointer): number;
  get_static_field_data_vtable(vtablePointer: MonoPointer): number;
  get_vtable(classPointer: MonoPointer): number;
  get_class_from_method(methodPointer: MonoPointer): MonoPointer;
  get_compile_method(className: string, methodName: string, parameterCount: number, assembly: string): number;
  unbox(objectPointer: MonoPointer): number;
  unbox_vec3(objectPointer: MonoPointer): Vector3 | null;
}

declare const store: {
  get_bool(key: string): boolean;
  set_bool(key: string, value: boolean): void;
  get_int(key: string): number;
  set_int(key: string, value: number): void;
  get_float(key: string): number;
  set_float(key: string, value: number): void;
};

declare const main_batch: {
  add(name: string, signature: string, callback: (address: number) => void): void;
  run(moduleName?: string | null): void;
  fullscan_run(): void;
  dispatch(): void;
};

declare module "mono" {
  export const mono: MonoApi;
  /** Class-style export for callers that prefer `Mono.get_class(...)`. */
  export class Mono {
    constructor();
    static get_root_domain: MonoApi["get_root_domain"];
    static get_class: MonoApi["get_class"];
    static get_method: MonoApi["get_method"];
    static get_field: MonoApi["get_field"];
    static invoke_method: MonoApi["invoke_method"];
    static set_field_int: MonoApi["set_field_int"];
    static set_field_float: MonoApi["set_field_float"];
    static set_field_string: MonoApi["set_field_string"];
    static get_field_value: MonoApi["get_field_value"];
    static get_field_offset: MonoApi["get_field_offset"];
    static get_static_field_value: MonoApi["get_static_field_value"];
    static get_static_field_data: MonoApi["get_static_field_data"];
    static get_static_field_data_vtable: MonoApi["get_static_field_data_vtable"];
    static get_vtable: MonoApi["get_vtable"];
    static get_class_from_method: MonoApi["get_class_from_method"];
    static get_compile_method: MonoApi["get_compile_method"];
    static unbox: MonoApi["unbox"];
    static unbox_vec3: MonoApi["unbox_vec3"];
  }
}

declare module "commands" {
  export function call(name: string): void;
}

declare module "canvas" {
  export interface MenuBuilder {
    add_bool(text: string, description?: string): void;
    add_slider(text: string, min: number, max: number, step: number): void;
    add_choose(text: string, values: string[], callback: (index: number, value: string) => void): void;
    add_option(text: string, callback?: () => void): void;
    add_submenu(text: string, submenuId: number, callback?: () => void): void;
  }
  export function add_tab(title: string, id: number, build: (menu: MenuBuilder) => void): void;
  export function add_submenu(title: string, id: number, build: (menu: MenuBuilder) => void): void;
}

declare module "command_types" {
  export class BoolCommand {
    get_state(): boolean;
    set_state(state: boolean): void;
    on_enable(callback: () => void): void;
    on_disable(callback: () => void): void;
  }
  export class LoopedCommand {
    get_state(): boolean;
    set_state(state: boolean): void;
    on_tick(callback: () => void): void;
    on_disable(callback: () => void): void;
  }
  export class CommandFactory {
    static create_bool(name: string, label: string, description: string, defaultState: boolean): BoolCommand;
    static create_looped(name: string, label: string, description: string): LoopedCommand;
  }
}
