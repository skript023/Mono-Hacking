declare module "detour" {
  export class NativeArgument {
    string(): string;
    monoString(): string | null;
    number(): number;
    float(): number;
    double(): number;
    valueOf(): number;
  }

  export class NativeContext {
    args(): number[];
    getArg(index: number): NativeArgument;
    setArg(index: number, value: number): void;
    setFloat(index: number, value: number): void;
    setDouble(index: number, value: number): void;
    callOriginal(): number;
  }

  export class Hook {
    static add(name: string, address: number, argumentCount: number, callback: (context: NativeContext) => number | void): boolean;
    static enable(name: string): void;
    static disable(name: string): void;
  }
}
