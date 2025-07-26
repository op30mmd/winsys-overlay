using System;
using System.Linq;
using LibreHardwareMonitor.Hardware;

public class UpdateVisitor : IVisitor
{
    public void VisitComputer(IComputer computer) 
    {
        computer.Traverse(this);
    }
    public void VisitHardware(IHardware hardware)
    {
        hardware.Update();
        foreach (IHardware subHardware in hardware.SubHardware)
            subHardware.Accept(this);
    }
    public void VisitSensor(ISensor sensor) { }
    public void VisitParameter(IParameter parameter) { }
}

public class Program
{
    public static void Main(string[] args)
    {
        Computer computer = new Computer
        {
            IsCpuEnabled = true,
            IsGpuEnabled = true
        };

        computer.Open();
        computer.Accept(new UpdateVisitor());

        float? cpuTemp = null;
        float? gpuTemp = null;

        foreach (var hardware in computer.Hardware)
        {
            foreach (var sensor in hardware.Sensors)
            {
                if (sensor.SensorType == SensorType.Temperature)
                {
                    // This is a simplified search. A more robust solution would handle multiple CPUs/GPUs.
                    if (hardware.HardwareType == HardwareType.Cpu && sensor.Name.Contains("Core"))
                    {
                        if (!cpuTemp.HasValue || sensor.Value > cpuTemp.Value)
                        {
                            cpuTemp = sensor.Value;
                        }
                    }
                    else if (hardware.HardwareType == HardwareType.GpuNvidia || hardware.HardwareType == HardwareType.GpuAmd)
                    {
                         if (sensor.Name.Contains("Core") || sensor.Name.Contains("Hotspot"))
                         {
                            if (!gpuTemp.HasValue || sensor.Value > gpuTemp.Value)
                            {
                                gpuTemp = sensor.Value;
                            }
                         }
                    }
                }
            }
        }

        Console.WriteLine($"CPU:{cpuTemp ?? -1:F1},GPU:{gpuTemp ?? -1:F1}");

        computer.Close();
    }
}
