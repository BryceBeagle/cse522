with Ada.Text_IO; use Ada.Text_IO;
with Ada.Command_Line;
with Ada.Numerics.Float_Random; use Ada.Numerics.Float_Random;


procedure Task_Generator is
	pragma Assertion_Policy(Dynamic_Predicate=>CHECK);

	type Distribution_Setting is (Wide, Tight);
	type File_Settings is record
		File : Ada.Text_IO.File_Type;
		Task_Number : Positive;
		Deadline_Range : Distribution_Setting;
	end record;

	type Fixed_Number is delta 0.01 range 0.0 .. 100.0;
	subtype Weird_Fixed_Number is Fixed_Number with Dynamic_Predicate => Weird_Fixed_Number > 40.0;

	type Task_Parameters is record
		Period : Fixed_Number := 0.0;
		Deadline : Fixed_Number := 0.0;
		WCET : Fixed_Number := 0.0;
	end record
		with Dynamic_Predicate =>
			Task_Parameters.Deadline in Task_Parameters.WCET .. Task_Parameters.Period;

	package F_IO is new Ada.Text_IO.Fixed_IO(Fixed_Number);
	package I_IO is new Ada.Text_IO.Integer_IO(Integer);

	procedure Print_Task_Parameters (F : File_Type; T : Task_Parameters) is
	begin
		F_IO.Put(F, T.WCET);
		F_IO.Put(F, T.Deadline);
		F_IO.Put(F, T.Period);
		New_Line(F);
	end Print_Task_Parameters;


	File_Name_Root : String := Ada.Command_Line.Argument(1);
	Task_Files : array (1 .. 4) of File_Settings;
	Sets : constant Integer := 5000;
	Random_Generator : Generator;

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

		for I in 1 .. Sets loop
			I_IO.Put(F.File, F.Task_Number, 0);
			New_Line(F.File);

			for T in 1 .. F.Task_Number loop

				declare
					New_Parameters : Task_Parameters;
				begin
					while New_Parameters.Period = 0.0 loop
						New_Parameters.Period := Fixed_Number(Random(Random_Generator) * 100.0);
					end loop;

					New_Parameters.WCET := New_Parameters.Period * Fixed_Number(Random(Random_Generator)) / 5;
					if New_Parameters.WCET = 0.0 then
						New_Parameters.WCET := 0.01;
					end if;

					if F.Deadline_Range = Tight then
						New_Parameters.Deadline :=
							((New_Parameters.Period - New_Parameters.WCET) / 2 * Fixed_Number(Random(Random_Generator)))
							+ (New_Parameters.WCET + (New_Parameters.Period - New_Parameters.WCET) / 2);
					else
						New_Parameters.Deadline := (New_Parameters.Period - New_Parameters.WCET) * Fixed_Number(Random(Random_Generator)) + New_Parameters.WCET;
					end if;

					Print_Task_Parameters(F.File, New_Parameters);

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

		end loop;

	end loop;

end Task_Generator;