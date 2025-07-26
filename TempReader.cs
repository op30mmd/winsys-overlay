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
            IsGpuEnabled = true,
            IsMotherboardEnabled = true
        };

        computer.Open();

        // Run in a loop to avoid constant restarting
        while (true)
        {
            try
            {
                // Wait for a command from the main application
                var line = Console.ReadLine();
                if (line == null || line.ToLower() == "exit")
                {
                    break;
                }

                if (line.ToLower() == "update")
                {
                    computer.Accept(new UpdateVisitor());

                    float? cpuTemp = null;
                    float? gpuTemp = null;

                    foreach (var hardware in computer.Hardware)
                    {
                        foreach (var sensor in hardware.Sensors)
                        {
                            if (sensor.SensorType == SensorType.Temperature && sensor.Value.HasValue)
                            {
                                string sensorName = sensor.Name.ToLower();
                                if (hardware.HardwareType == HardwareType.Cpu && 
                                    (sensorName.Contains("core") || sensorName.Contains("package") || sensorName.Contains("cpu") || 
                                     sensorName.Contains("tctl") || sensorName.Contains("tdie") || sensorName.Contains("ccd")))
                                {
                                    if (!cpuTemp.HasValue || sensor.Value > cpuTemp.Value) cpuTemp = sensor.Value;
                                }
                                else if (hardware.HardwareType == HardwareType.Motherboard && sensorName.Contains("cpu") &&
                                         !sensorName.Contains("fan") && !sensorName.Contains("pump"))
                                {
                                    if (!cpuTemp.HasValue || sensor.Value > cpuTemp.Value) cpuTemp = sensor.Value;
                                }
                                else if (hardware.HardwareType == HardwareType.GpuNvidia || hardware.HardwareType == HardwareType.GpuAmd || hardware.HardwareType == HardwareType.GpuIntel)
                                {
                                    if (sensorName.Contains("core") || sensorName.Contains("gpu") || sensorName.Contains("hotspot") || 
                                        sensorName.Contains("junction") || sensorName.Contains("memory"))
                                    {
                                        if (!gpuTemp.HasValue || sensor.Value > gpuTemp.Value) gpuTemp = sensor.Value;
                                    }
                                }
                            }
                        }
                    }
                    
                    Console.WriteLine($"CPU:{cpuTemp ?? -1:F1},GPU:{gpuTemp ?? -1:F1}");
                }
            }
            catch (Exception)
            {
                // Exit gracefully if the parent process closes the pipe
                break;
            }
        }
        computer.Close();
    }
}
