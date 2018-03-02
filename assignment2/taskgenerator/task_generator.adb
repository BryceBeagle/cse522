with Ada.Text_IO; use Ada.Text_IO;
with Ada.Command_Line;
with Ada.Numerics.Float_Random; use Ada.Numerics.Float_Random;
with Ada.Containers.Generic_Array_Sort;

procedure Task_Generator is
	pragma Assertion_Policy(Dynamic_Predicate=>CHECK);

	Random_Generator : Generator;

	type Distribution_Setting is (Wide, Tight);
	type File_Settings is record
		File : Ada.Text_IO.File_Type;
		Task_Number : Positive;
		Deadline_Range : Distribution_Setting;
	end record;

	type Fixed_Number is delta 0.01 digits 6 range 0.0 .. 1000.0;
	type Fixed_Number_Array is array (Positive range <>) of Fixed_Number;

	type Task_Parameters is record
		Period : Fixed_Number := 0.0;
		Deadline : Fixed_Number := 0.0;
		WCET : Fixed_Number := 0.0;
	end record
		with Dynamic_Predicate =>
			Task_Parameters.Deadline in Task_Parameters.WCET .. Task_Parameters.Period;

	package F_IO is new Ada.Text_IO.Decimal_IO(Fixed_Number);
	package I_IO is new Ada.Text_IO.Integer_IO(Integer);

	procedure Print_Task_Parameters (F : File_Type; T : Task_Parameters) is
	begin
		F_IO.Put(F, T.WCET);
		F_IO.Put(F, T.Deadline);
		F_IO.Put(F, T.Period);
		New_Line(F);
	end Print_Task_Parameters;

	function U_Uni_Fast (N : Positive; Util : Fixed_Number) return Fixed_Number_Array is
		procedure Fixed_Sort is new Ada.Containers.Generic_Array_Sort (Positive, Fixed_Number, Fixed_Number_Array);
		Return_Array : Fixed_Number_Array(1 .. N);
		Temp_Array : Fixed_Number_Array(1 .. N + 1);
	begin
		Temp_Array(1) := 0.0;
		for I in 2 .. N loop
			Temp_Array(I) := Fixed_Number(Random(Random_Generator)) * Util;
		end loop;
		Temp_Array(N+1) := Util;
		Fixed_Sort(Temp_Array);

		for I in 1 .. N loop
			Return_Array(I) := Temp_Array(I+1) - Temp_Array(I);
		end loop;

		return Return_Array;
	end U_Uni_Fast;

	function Get_Task_Periods (N : Positive) return Fixed_Number_Array is
		Return_Array : Fixed_Number_Array (1 .. N);
	begin
		for I in 1 .. N loop
			case Integer(Random(Random_Generator)) * 100 is
				when 0 .. 33 => Return_Array(I) := 10 * Fixed_Number(Random(Random_Generator));
				when 34 .. 66 => Return_Array(I) := 100 * Fixed_Number(Random(Random_Generator));
				when 67 .. 100 => Return_Array(I) := 1000 * Fixed_Number(Random(Random_Generator));
				when others => Return_Array(I) := 10 * Fixed_Number(Random(Random_Generator));
			end case;
			if Return_Array(I) = 0.0 then
				Return_Array(I) := 0.1;
			end if;
		end loop;
		return Return_Array;
	end Get_Task_Periods;


	File_Name_Root : String := Ada.Command_Line.Argument(1);
	Task_Files : array (1 .. 4) of File_Settings;
	Sets : constant Integer := 50_000;
	Util_Counter : Fixed_Number;

begin

	Create(File=>Task_Files(1).File, Name=>File_Name_Root & "_10_Wide");
	Task_Files(1).Task_Number := 10;
	Task_Files(1).Deadline_Range := Wide;

	Create(File=>Task_Files(2).File, Name=>File_Name_Root & "_25_Wide");
	Task_Files(2).Task_Number := 25;
	Task_Files(2).Deadline_Range := Wide;

	Create(File=>Task_Files(3).File, Name=>File_Name_Root & "_10_Tight");
	Task_Files(3).Task_Number := 10;
	Task_Files(3).Deadline_Range := Tight;

	Create(File=>Task_Files(4).File, Name=>File_Name_Root & "_25_Tight");
	Task_Files(4).Task_Number := 25;
	Task_Files(4).Deadline_Range := Tight;

	Reset(Random_Generator);

	for F of Task_Files loop
		I_IO.Put(F.File, Sets, 0);
		New_Line(F.File);
		Util_Counter := 0.05;

		for U in 1 .. 10 loop

			for I in 1 .. Sets/10 loop

				I_IO.Put(F.File, F.Task_Number, 0);
				New_Line(F.File);

				declare
					New_Parameters : Task_Parameters;
					Period : Fixed_Number_Array := Get_Task_Periods(F.Task_Number);
					Utilization : Fixed_Number_Array := U_Uni_Fast(F.Task_Number, Util_Counter);
				begin

					for T in 1 .. F.Task_Number loop

						New_Parameters.Period := Period(T);
						New_Parameters.WCET := Period(T)*Utilization(T);

						if F.Deadline_Range = Tight then
							New_Parameters.Deadline :=
								((New_Parameters.Period - New_Parameters.WCET) / 2 * Fixed_Number(Random(Random_Generator)))
								+ (New_Parameters.WCET + (New_Parameters.Period - New_Parameters.WCET) / 2);
						else
							New_Parameters.Deadline := (New_Parameters.Period - New_Parameters.WCET) * Fixed_Number(Random(Random_Generator)) + New_Parameters.WCET;
						end if;

						Print_Task_Parameters(F.File, New_Parameters);

					end loop;

				exception
					when others =>
						Put_Line("Values Invalid");
						Put("Period ");
						F_IO.Put(New_Parameters.Period);
						Put("Deadline ");
						F_IO.Put(New_Parameters.Deadline);
						Put("WCET ");
						F_IO.Put(New_Parameters.WCET);
						New_Line;
						return;
				end;

			end loop;

			Util_Counter := Util_Counter + 0.1;

		end loop;

	end loop;

end Task_Generator;