declare module "detour" {
	export interface NativeContext {
		getArg(index: number): { number(): number };
		callOriginal(): number;
	}

	export class Hook {
		static add(name: string, address: number, argumentCount: number, callback: (context: NativeContext) => number): boolean;
		static enable(name: string): void;
		static disable(name: string): void;
	}
}
