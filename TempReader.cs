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
            IsMotherboardEnabled = true  // Enable motherboard sensors for additional temp readings
        };

        computer.Open();
        computer.Accept(new UpdateVisitor());

        float? cpuTemp = null;
        float? gpuTemp = null;

        // Debug output to see what's available
        bool debug = args.Length > 0 && args[0] == "--debug";
        
        foreach (var hardware in computer.Hardware)
        {
            if (debug)
            {
                Console.WriteLine($"Hardware: {hardware.Name} ({hardware.HardwareType})");
            }
            
            foreach (var sensor in hardware.Sensors)
            {
                if (debug)
                {
                    Console.WriteLine($"  Sensor: {sensor.Name} ({sensor.SensorType}) = {sensor.Value}");
                }
                
                if (sensor.SensorType == SensorType.Temperature && sensor.Value.HasValue)
                {
                    // CPU Temperature Detection - More comprehensive approach
                    if (hardware.HardwareType == HardwareType.Cpu)
                    {
                        // Look for various CPU temperature sensor names
                        string sensorName = sensor.Name.ToLower();
                        if (sensorName.Contains("core") || 
                            sensorName.Contains("package") || 
                            sensorName.Contains("cpu") ||
                            sensorName.Contains("tctl") ||  // AMD Ryzen
                            sensorName.Contains("tdie") ||  // AMD Ryzen
                            sensorName.Contains("ccd"))     // AMD Ryzen chiplet
                        {
                            if (!cpuTemp.HasValue || sensor.Value > cpuTemp.Value)
                            {
                                cpuTemp = sensor.Value;
                            }
                        }
                    }
                    // Also check motherboard sensors for CPU temperatures
                    else if (hardware.HardwareType == HardwareType.Motherboard)
                    {
                        string sensorName = sensor.Name.ToLower();
                        if (sensorName.Contains("cpu") && 
                            !sensorName.Contains("fan") && 
                            !sensorName.Contains("pump"))
                        {
                            if (!cpuTemp.HasValue || sensor.Value > cpuTemp.Value)
                            {
                                cpuTemp = sensor.Value;
                            }
                        }
                    }
                    // GPU Temperature Detection
                    else if (hardware.HardwareType == HardwareType.GpuNvidia || 
                             hardware.HardwareType == HardwareType.GpuAmd ||
                             hardware.HardwareType == HardwareType.GpuIntel)
                    {
                        string sensorName = sensor.Name.ToLower();
                        if (sensorName.Contains("core") || 
                            sensorName.Contains("gpu") || 
                            sensorName.Contains("hotspot") ||
                            sensorName.Contains("junction") ||
                            sensorName.Contains("memory"))
                        {
                            if (!gpuTemp.HasValue || sensor.Value > gpuTemp.Value)
                            {
                                gpuTemp = sensor.Value;
                            }
                        }
                    }
                }
            }
            
            // Also check sub-hardware (like individual CPU cores)
            foreach (var subHardware in hardware.SubHardware)
            {
                if (debug)
                {
                    Console.WriteLine($"  SubHardware: {subHardware.Name} ({subHardware.HardwareType})");
                }
                
                foreach (var sensor in subHardware.Sensors)
                {
                    if (debug)
                    {
                        Console.WriteLine($"    Sensor: {sensor.Name} ({sensor.SensorType}) = {sensor.Value}");
                    }
                    
                    if (sensor.SensorType == SensorType.Temperature && sensor.Value.HasValue)
                    {
                        if (subHardware.HardwareType == HardwareType.Cpu)
                        {
                            string sensorName = sensor.Name.ToLower();
                            if (sensorName.Contains("core") || 
                                sensorName.Contains("package") || 
                                sensorName.Contains("cpu") ||
                                sensorName.Contains("tctl") ||
                                sensorName.Contains("tdie") ||
                                sensorName.Contains("ccd"))
                            {
                                if (!cpuTemp.HasValue || sensor.Value > cpuTemp.Value)
                                {
                                    cpuTemp = sensor.Value;
                                }
                            }
                        }
                    }
                }
            }
        }

        // If still no CPU temp found, try a more aggressive search
        if (!cpuTemp.HasValue)
        {
            foreach (var hardware in computer.Hardware)
            {
                foreach (var sensor in hardware.Sensors)
                {
                    if (sensor.SensorType == SensorType.Temperature && sensor.Value.HasValue)
                    {
                        string sensorName = sensor.Name.ToLower();
                        string hardwareName = hardware.Name.ToLower();
                        
                        // Look for any temperature sensor that might be CPU-related
                        if ((sensorName.Contains("temp") && (hardwareName.Contains("cpu") || hardwareName.Contains("processor"))) ||
                            (sensorName.Contains("thermal") && !sensorName.Contains("gpu")) ||
                            sensorName.StartsWith("temp1") || // Common motherboard CPU temp sensor
                            sensorName.StartsWith("temp2"))   // Another common CPU temp sensor
                        {
                            if (!cpuTemp.HasValue || sensor.Value > cpuTemp.Value)
                            {
                                cpuTemp = sensor.Value;
                            }
                        }
                    }
                }
            }
        }

        if (debug)
        {
            Console.WriteLine($"Final CPU Temp: {cpuTemp}");
            Console.WriteLine($"Final GPU Temp: {gpuTemp}");
        }
        else
        {
            Console.WriteLine($"CPU:{cpuTemp ?? -1:F1},GPU:{gpuTemp ?? -1:F1}");
        }

        computer.Close();
    }
}
